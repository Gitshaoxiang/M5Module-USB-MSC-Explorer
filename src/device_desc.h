#ifndef _DEVICE_DESC_H_
#define _DEVICE_DESC_H_

#include "device_def.h"

void print_lsusb(void);
void print_device_descriptor(tuh_xfer_t *xfer);
void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len);

#endif