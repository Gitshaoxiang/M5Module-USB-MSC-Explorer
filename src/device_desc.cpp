

#include "device_desc.h"

extern dev_info_t dev_info[CFG_TUH_DEVICE_MAX];

void print_lsusb(void) {
    bool no_device = true;
    for (uint8_t daddr = 1; daddr < CFG_TUH_DEVICE_MAX + 1; daddr++) {
        // TODO can use tuh_mounted(daddr), but tinyusb has an bug
        // use local connected flag instead
        dev_info_t *dev = &dev_info[daddr - 1];
        if (dev->mounted) {
            Serial.printf("Device %u: ID %04x:%04x %s %s\r\n", daddr,
                          dev->desc_device.idVendor, dev->desc_device.idProduct,
                          (char *)dev->manufacturer, (char *)dev->product);

            no_device = false;
        }
    }

    if (no_device) {
        Serial.println("No device connected (except hub)");
    }
}

void print_device_descriptor(tuh_xfer_t *xfer) {
    if (XFER_RESULT_SUCCESS != xfer->result) {
        Serial.printf("Failed to get device descriptor\r\n");
        return;
    }

    uint8_t const daddr      = xfer->daddr;
    dev_info_t *dev          = &dev_info[daddr - 1];
    tusb_desc_device_t *desc = &dev->desc_device;

    Serial.printf("Device %u: ID %04x:%04x\r\n", daddr, desc->idVendor,
                  desc->idProduct);
    Serial.printf("Device Descriptor:\r\n");
    Serial.printf("  bLength             %u\r\n", desc->bLength);
    Serial.printf("  bDescriptorType     %u\r\n", desc->bDescriptorType);
    Serial.printf("  bcdUSB              %04x\r\n", desc->bcdUSB);
    Serial.printf("  bDeviceClass        %u\r\n", desc->bDeviceClass);
    Serial.printf("  bDeviceSubClass     %u\r\n", desc->bDeviceSubClass);
    Serial.printf("  bDeviceProtocol     %u\r\n", desc->bDeviceProtocol);
    Serial.printf("  bMaxPacketSize0     %u\r\n", desc->bMaxPacketSize0);
    Serial.printf("  idVendor            0x%04x\r\n", desc->idVendor);
    Serial.printf("  idProduct           0x%04x\r\n", desc->idProduct);
    Serial.printf("  bcdDevice           %04x\r\n", desc->bcdDevice);

    // Get String descriptor using Sync API
    Serial.printf("  iManufacturer       %u     ", desc->iManufacturer);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_manufacturer_string_sync(
            daddr, LANGUAGE_ID, dev->manufacturer, sizeof(dev->manufacturer))) {
        utf16_to_utf8(dev->manufacturer, sizeof(dev->manufacturer));
        Serial.printf((char *)dev->manufacturer);
    }
    Serial.printf("\r\n");

    Serial.printf("  iProduct            %u     ", desc->iProduct);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_product_string_sync(daddr, LANGUAGE_ID, dev->product,
                                               sizeof(dev->product))) {
        utf16_to_utf8(dev->product, sizeof(dev->product));
        Serial.printf((char *)dev->product);
    }
    Serial.printf("\r\n");

    Serial.printf("  iSerialNumber       %u     ", desc->iSerialNumber);
    if (XFER_RESULT_SUCCESS ==
        tuh_descriptor_get_serial_string_sync(daddr, LANGUAGE_ID, dev->serial,
                                              sizeof(dev->serial))) {
        utf16_to_utf8(dev->serial, sizeof(dev->serial));
        Serial.printf((char *)dev->serial);
    }
    Serial.printf("\r\n");

    Serial.printf("  bNumConfigurations  %u\r\n", desc->bNumConfigurations);

    // print device summary
    print_lsusb();
}

//--------------------------------------------------------------------+
// String Descriptor Helper
//--------------------------------------------------------------------+

static void _convert_utf16le_to_utf8(const uint16_t *utf16, size_t utf16_len,
                                     uint8_t *utf8, size_t utf8_len) {
    // TODO: Check for runover.
    (void)utf8_len;
    // Get the UTF-16 length out of the data itself.

    for (size_t i = 0; i < utf16_len; i++) {
        uint16_t chr = utf16[i];
        if (chr < 0x80) {
            *utf8++ = chr & 0xff;
        } else if (chr < 0x800) {
            *utf8++ = (uint8_t)(0xC0 | (chr >> 6 & 0x1F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        } else {
            // TODO: Verify surrogate.
            *utf8++ = (uint8_t)(0xE0 | (chr >> 12 & 0x0F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 6 & 0x3F));
            *utf8++ = (uint8_t)(0x80 | (chr >> 0 & 0x3F));
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
}

// Count how many bytes a utf-16-le encoded string will take in utf-8.
static int _count_utf8_bytes(const uint16_t *buf, size_t len) {
    size_t total_bytes = 0;
    for (size_t i = 0; i < len; i++) {
        uint16_t chr = buf[i];
        if (chr < 0x80) {
            total_bytes += 1;
        } else if (chr < 0x800) {
            total_bytes += 2;
        } else {
            total_bytes += 3;
        }
        // TODO: Handle UTF-16 code points that take two entries.
    }
    return total_bytes;
}

void utf16_to_utf8(uint16_t *temp_buf, size_t buf_len) {
    size_t utf16_len = ((temp_buf[0] & 0xff) - 2) / sizeof(uint16_t);
    size_t utf8_len  = _count_utf8_bytes(temp_buf + 1, utf16_len);

    _convert_utf16le_to_utf8(temp_buf + 1, utf16_len, (uint8_t *)temp_buf,
                             buf_len);
    ((uint8_t *)temp_buf)[utf8_len] = '\0';
}
