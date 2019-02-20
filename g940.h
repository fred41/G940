#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>
#include <stdlib.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>

const size_t G940_INTERFACE = 0;
const size_t G940_ENDPOINT = 1;
const size_t G940_VENDOR_ID = 0x046d;
const size_t G940_PRODUCT_ID = 0xc287;
const size_t G940_REPORT_SIZE = 21; //????
const size_t G940_FF_REPORT_SIZE = 64;
const size_t G940_READ_TIMEOUT = 0;

typedef struct {
	int8_t x;
	int8_t y;
} hat_xy;

typedef struct {
	int16_t	constant;
	int32_t	pad0;
	int16_t	spring_deadzone_neg;
	int16_t	spring_deadzone_pos;
	int8_t	spring_coeff_neg;
	int8_t	spring_coeff_pos;
	int16_t	spring_saturation;
	int32_t	pad1;
	int32_t	pad2;
	int8_t	damper_coeff_neg;
	int8_t	damper_coeff_pos;
	int16_t	damper_saturation;
	int32_t	pad3;
} __attribute__((packed, aligned(1))) ff_axis_t;

typedef struct {
	ff_axis_t x;
	ff_axis_t y;
} __attribute__((packed, aligned(1))) ff_t;
