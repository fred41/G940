#include "g940.h"

using namespace std;

// lookup-table for BTN codes
const uint16_t btn_code_lut[32] = {
// all buttons, sensors and switches
	BTN_TRIGGER,			// main trigger, first phase
	BTN_THUMB,				// red 'fire'
	BTN_TOP,					// S1
	BTN_TOP2,					// S2
	BTN_THUMB2,				// S3
	BTN_BASE,					// S4
	BTN_BASE2,				// S5
	0,

	BTN_PINKIE,				// main trigger, second phase
	BTN_BASE3,				// T1
	BTN_BASE4,				// T2
	BTN_BASE5,				// T3
	BTN_BASE6,				// T4
	BTN_TRIGGER_HAPPY1,			// P1
	BTN_TRIGGER_HAPPY2,			// P2
	BTN_TRIGGER_HAPPY3,			// P3

	BTN_TRIGGER_HAPPY4,			// P4
	BTN_TRIGGER_HAPPY5,			// P5
	BTN_TRIGGER_HAPPY6,			// P6
	BTN_TRIGGER_HAPPY7,			// P7
	BTN_TRIGGER_HAPPY8,			// P8
	BTN_TRIGGER_HAPPY9,			// MODE Switch 1
	BTN_TRIGGER_HAPPY10,		// MODE Switch 3
	0,											// fix at logical 1

	0,
	0,
	0,
	0,
	0,
	BTN_DEAD,				// optical sensor on main stick
	0,
	0					// external power supply connected, not mapped to event
};

// the HAT looup-table, extended to 16 just to be safe
const hat_xy hat_lut[16] = {
	{0,-1},
	{1,-1},
	{1,0},
	{1,1},
	{0,1},
	{-1,1},
	{-1,0},
	{-1,-1},
	{0,0},
	{0,0},
	{0,0},
	{0,0},
	{0,0},
	{0,0},
	{0,0},
	{0,0}
};

const uint8_t axes_code_lut[20] = {
// main axes x,y, 10bit each, -512..511
	ABS_X,
	ABS_Y,
// 3 x HAT, 4bit each, 8 directions
	ABS_HAT0X,
	ABS_HAT0Y,
	ABS_HAT1X,
	ABS_HAT1Y,
	ABS_HAT2X,
	ABS_HAT2Y,
// 2 x Throttle, 8bit each, 0..255
	ABS_THROTTLE,
	ABS_GAS,
// Rudder and 2 x brakes, 8bit each, 0..255
	ABS_RUDDER,
	ABS_BRAKE,
	ABS_WHEEL,
// 3 x TRIM, 8bit each, 0..255
	ABS_RZ,
	ABS_RY,
	ABS_RX,
// 2 x R1,R2 Wheels, 8bit each, 0..255
	ABS_PRESSURE,
	ABS_DISTANCE,
// Ministick x,y, 8bit each, 0..255
	ABS_TILT_X,
	ABS_TILT_Y
};

libusb_device_handle *g940_handle = NULL;
libusb_context *usb_ctx = NULL;
int uinput_fd = 0;


void g940_find_usb_dev(libusb_device **devs, size_t cnt, libusb_device **g940_dev, libusb_device_handle **g940_handle)
{

	libusb_device_descriptor desc;
	libusb_device *dev = NULL;
	libusb_device_handle *handle = NULL;

	for (int i=0; i<cnt; i++) {
		int ret = libusb_get_device_descriptor(devs[i], &desc);
		if (ret < 0) {
			printf("libusb_get_device_descriptor failed: %d\n", ret);
			return;
		}
		if (desc.idVendor == G940_VENDOR_ID && desc.idProduct == G940_PRODUCT_ID) {
			int ret = libusb_open(devs[i], &handle);
			if (ret != 0) {
				printf("libusb_open failed: %d\n", ret);
				return;
			}

			dev = devs[i];
			printf("found G940 device!\n");

			if (libusb_kernel_driver_active(handle, G940_INTERFACE) == 1) {
				ret = libusb_detach_kernel_driver(handle, G940_INTERFACE);
				if (ret < 0) {
					printf("libusb_detach_kernel_driver failed: %d\n", ret);
					return;
				}
			}

			ret = libusb_claim_interface(handle, G940_INTERFACE);
			if (ret < 0) {
				printf("libusb_claim_interface failed: %d\n", ret);
				return;
			}
		}
	}

	if (handle == NULL || dev == NULL)
		printf("No G940 device found!\n");
	else {
		*g940_dev = dev;
		*g940_handle = handle;
	}
}

void g940_init_device()
{
	uint8_t ff_buf[68] = {0};
	ff_t *ff = (ff_t*)&ff_buf[4];
	ff_axis_t *ff_axis;
	int size;

	// report ID
	ff_buf[3] = 2;

	// set usable conditions for horizontal ff axis
	ff_axis = &ff->x;
	ff_axis->spring_deadzone_neg = 0;
	ff_axis->spring_deadzone_pos = 0;
	ff_axis->spring_coeff_neg = 0x1f;
	ff_axis->spring_coeff_pos = 0x1f;
	ff_axis->spring_saturation = 0x1fff;
	ff_axis->damper_coeff_neg = 0x1f;
	ff_axis->damper_coeff_pos = 0x1f;
	ff_axis->damper_saturation = 0x1fff;

	// do the same for vertical axis
	memcpy(&ff->y, &ff->x, sizeof(ff_axis_t));

	if (libusb_interrupt_transfer(g940_handle, LIBUSB_ENDPOINT_OUT | G940_ENDPOINT,
	                              &ff_buf[3], G940_FF_REPORT_SIZE, &size, G940_READ_TIMEOUT)) {

		printf("libusb_interrupt_transfer failed");
	} else {

		if (size != G940_FF_REPORT_SIZE)
			printf("unexpected transfer size : %d\n", size);
	}
}

int g940_create_uinput()
{
	struct uinput_setup uinp;
	struct uinput_abs_setup uabs;
	const char* dev_ui_fn =
	  access("/dev/uinput", W_OK) == 0 ? "/dev/uinput" : 0;
	if (!dev_ui_fn) {
		printf("No writable uinput device found\n");
		return 0;
	}

	int fd = open(dev_ui_fn, O_WRONLY | O_NONBLOCK);
	if (fd <= 0) {
		printf("Opening uinput device failed: %d\n", fd);
		return 0;
	}

	ioctl(fd, UI_SET_EVBIT, EV_ABS);

	memset(&uinp, 0, sizeof(uinp));
	strcpy(uinp.name, "Flight System G940");
	uinp.id.version = 1;
	uinp.id.bustype = BUS_USB;
	uinp.id.product = G940_PRODUCT_ID;
	uinp.id.vendor = G940_VENDOR_ID;

	memset(&uabs, 0, sizeof(uabs));
	uabs.absinfo.minimum = -512;
	uabs.absinfo.maximum = 511;
	uabs.absinfo.fuzz = 0;
	uabs.absinfo.flat = 0;
	uabs.code = ABS_X;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_Y;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.absinfo.minimum = 0;
	uabs.absinfo.maximum = 255;
	uabs.code = ABS_RX;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_RY;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_RZ;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_THROTTLE;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_RUDDER;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_WHEEL;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_GAS;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_BRAKE;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_PRESSURE;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_DISTANCE;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_TILT_X;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_TILT_Y;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.absinfo.minimum = -1;
	uabs.absinfo.maximum = 1;
	uabs.code = ABS_HAT0X;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_HAT0Y;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_HAT1X;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_HAT1Y;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_HAT2X;
	ioctl(fd, UI_ABS_SETUP, &uabs);
	uabs.code = ABS_HAT2Y;
	ioctl(fd, UI_ABS_SETUP, &uabs);

	ioctl(fd, UI_SET_EVBIT, EV_KEY);

	// 9 x Stickbuttons and 4 x Throttlebuttons
	for (int i = BTN_TRIGGER; i < BTN_GAMEPAD; i++)
		ioctl(fd, UI_SET_KEYBIT, i);

	//P1-P8 and 2 x Modeswitch
	for (int i = BTN_TRIGGER_HAPPY1; i < BTN_TRIGGER_HAPPY11; i++)
		ioctl(fd, UI_SET_KEYBIT, i);

	int ret;
	if (ret = ioctl(fd, UI_DEV_SETUP, &uinp))
		printf("ioctl UI_DEV_SETUP failed: %d\n", ret);

	if (ret = ioctl(fd, UI_DEV_CREATE)) {
		printf("ioctl UI_DEV_CREATE failed: %d\n", ret);
		return -1;
	}
	return fd;
}


void cleanup()
{
	ioctl(uinput_fd, UI_DEV_DESTROY);
	close(uinput_fd);
	libusb_release_interface(g940_handle, G940_INTERFACE);
	libusb_close(g940_handle);
	libusb_exit(usb_ctx);
}

void sig_cb_handler(int signum)
{
	cleanup();
	exit(signum);
}

void g940_process_report(int ui_fd, uint32_t* state_buf)
{
	uint32_t *state_diff = &state_buf[6], *state = &state_buf[11];
	uint32_t mask;
	int i, j;
	struct input_event event;
//	struct timespec ts, te;

//	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);

	for (i=0; i<5; i++) {
		state_diff[i] = state[i] ^ state_buf[i + 1];
		state[i] = state_buf[i + 1];
	}

	gettimeofday(&event.time, 0);

	event.type = EV_ABS;
	// process 32bit wise, then refine, for performance
	// main axes X, Y
	if (*state_diff) {
		// main axes X, Y
		if (*state_diff & 0b1111111111) {
			event.code = axes_code_lut[0];
			event.value = (*state & 0b1111111111) << 22;
			event.value /= 0x400000;
			write(ui_fd, &event, sizeof(event));
		}
		if (*state_diff & 0b11111111110000000000) {
			event.code = axes_code_lut[1];
			event.value = (*state & 0b11111111110000000000) << 12;
			event.value /= 0x400000;
			write(ui_fd, &event, sizeof(event));
		}
		// HAT0, HAT1, HAT2
		if (*state_diff & 0xfff00000) {
			mask = 0x00f00000;
			for (int i=0; i<3; i++) {
				if (*state_diff & mask) {
					uint8_t index = (*state & mask) >> (i * 4 + 20);
					event.code = axes_code_lut[i * 2 + 2];
					event.value = hat_lut[index].x;
					write(ui_fd, &event, sizeof(event));
					event.code = axes_code_lut[i * 2 + 3];
					event.value = hat_lut[index].y;
					write(ui_fd, &event, sizeof(event));
				}
				mask = mask << 4;
			}
		}
	}

	// THROTTLE 1&2, RUDDER, BRAKE RIGHT
	// BRAKE LEFT, TRIM 3&2&1
	// R1, R2 , MINI_X, MINI_Y
	for (i=0; i<3; i++) {
		state_diff++;
		state++;
		if (*state_diff) {
			mask = 0xff;
			for (j=0; j<4; j++) {
				if (*state_diff & mask) {
					event.code = axes_code_lut[i * 4 + j + 8];
					event.value = (*state & mask) >> (j * 8);
					write(ui_fd, &event, sizeof(event));
				}
				mask = mask << 8;
			}
		}
	}

	// all the other buttons, sensors and switches
	if (++state_diff) {
		state++;
		mask = 1;
		event.type = EV_KEY;
		for (i=0; i<32; i++) {
			if(*state_diff & mask) {
				event.code = btn_code_lut[i];
				if(event.code) {
					event.value = (*state & mask) >> i;
					write(ui_fd, &event, sizeof(event));
				}
			}
			mask = mask << 1;
		}
	}

	event.type = EV_SYN;
	event.code = SYN_REPORT;
	event.value = 0;
	write(ui_fd, &event, sizeof(event));

//	clock_gettime(CLOCK_MONOTONIC_RAW, &te);
//	double dur = ((double)te.tv_nsec - (double)ts.tv_nsec) / 1000;
//	printf("us: %f\n",dur);

}

int main(int argc, char** argv)
{

	// one complete cache line
	static uint32_t state_buf[16] __attribute__((aligned(64)))= {0};
	libusb_device **devs = NULL;
	libusb_device *g940_dev = NULL;

	size_t cnt = 0;
	int ret = 0, size;
	bool running = true;

	ret = libusb_init(&usb_ctx);
	if (ret < 0) {
		printf("libusb_init failed: %d\n", ret);
		return 1;
	}

	cnt = libusb_get_device_list(usb_ctx, &devs);
	if (cnt < 0) {
		printf("libusb_get_device_list failed: %d\n", cnt);
		return 1;
	}
	printf("device count: %d\n", cnt);

	g940_find_usb_dev(devs, cnt, &g940_dev, &g940_handle);

	libusb_free_device_list(devs, 1);

	if (!g940_dev) {
		return 1;
	}

	g940_init_device();

	uinput_fd = g940_create_uinput();
	if (!uinput_fd)
		return 1;

	signal(SIGINT, sig_cb_handler);

	do {
		if (libusb_interrupt_transfer(g940_handle, LIBUSB_ENDPOINT_IN | G940_ENDPOINT,
		                              (uint8_t*)state_buf + 3, G940_REPORT_SIZE, &size, G940_READ_TIMEOUT)) {

			printf("libusb_interrupt_transfer failed");
			running = false;
		} else {

			if (size != G940_REPORT_SIZE)
				printf("unexpected report size : %d\n", size);

			else
				g940_process_report(uinput_fd, state_buf);
		}

	} while (running);

	cleanup();

	return EXIT_SUCCESS;
}
