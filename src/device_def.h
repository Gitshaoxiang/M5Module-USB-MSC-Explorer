#ifndef _DEVICE_DEF_H_
#define _DEVICE_DEF_H_

#include "Adafruit_TinyUSB.h"
#include "FS.h"
#include "SdFat.h"

#define LANGUAGE_ID 0x0409

typedef struct {
    tusb_desc_device_t desc_device;
    uint16_t manufacturer[32];
    uint16_t product[48];
    uint16_t serial[16];
    bool mounted;
} dev_info_t;

typedef struct {
    String name;
    uint32_t size;
    bool isDir;
} file_info_t;

#endif