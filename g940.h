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
const size_t G940_ENDPOINT = 0x81;
const size_t G940_VENDOR_ID = 0x046d;
const size_t G940_PRODUCT_ID = 0xc287;
const size_t G940_REPORT_SIZE = 21; //????
const size_t G940_READ_TIMEOUT = 0;

struct hat_xy {
	int8_t x;
	int8_t y;
};
