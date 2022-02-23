/*
    Copyright (c) 2022 Jean THOMAS.

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the Software
    is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
    OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <libusb.h>

/* 0xB042-0xB044 is XDFP (DSP interface) data in  */
/* 0xB045-0xB046 is XDFP address/command interface */
/* 0xB05C-0xB05E is XDFP data out (unused here) */
#define V8_WRITE_START_ADDR   0xb042

/* This memory address is mapped to the RAM */
#define V8_PLUGIN_START_ADDR  0x8120
#define XDFP_STARTING_EQ_ADDR 0x50

#define EQ_TABLE_SIZE       16

#define kMicronasSetMemReq      4
#define kMicronasGetMemReq      5136

enum TrinityAvailablePower {
  POWER_NULL, POWER_500MA, POWER_1500MA, POWER_3000MA, POWER_4000MA
};

/* These EQ come from AppleUSBTrinityAudioDevice.cpp */
static const int32_t power4AEQSettings[] = {
    228,  -129968,  130513,
    -279, -125942,  128415,
    -1689,  -123355,  126686,
    -5136,  -95891,   109553,
    -18995, -993,   6924,
    -45000
};

static const int32_t    power3AEQSettings[] = {
    228,  -129968,  130513,
    -279, -125942,  128415,
    -1689,  -123355,  126686,
    -5137,  -95891,   109553,
    -18995, -993,   6924,
    -42000};

static const int32_t power1500mAEQSettings[] = {
  228,  -129968,  130513, 
  -279, -125942,  128415, 
  -1689,  -123355,  126686, 
  -5137,  -95891,   109553, 
  -18995, -993,   6924, 
  -20000};

static const int32_t power500mAEQSettings[] = {
    228,  -129968,  130513,
    -279, -125942,  128415,
    -1689,  -123355,  126686,
    -5137,  -95891,   109553,
    -18995, -993,   6924,
    -8000};

/* Plugins are firmware extensions. The plugin is necessary to enable the amplifier chip. */
static const uint8_t pluginBinary[] = {
    0xBF, 0x35, 0x81, 0xBA, 0x85, 0xEA, 0x7B, 0x80, 0xE1, 0x13, 0xBF, 0xDE, 0x0B, 0x8D, 0xB9, 0x85,
    0xBF, 0x1A, 0x0C, 0x8D, 0xB9, 0xE9, 0xF3, 0x81, 0xE8, 0x80, 0x80, 0x79, 0x90, 0x03, 0xBC, 0xEC,
    0x81, 0xEA, 0xA2, 0xB0, 0xE4, 0x00, 0xE5, 0x02, 0x44, 0x99, 0x01, 0x45, 0x15, 0x71, 0x14, 0x19,
    0x90, 0xF6, 0xE0, 0xF0, 0xC8, 0x87, 0x80, 0xC8, 0x51, 0xB0, 0x12, 0x63, 0x90, 0x03, 0x28, 0x98,
    0x02, 0xE0, 0x40, 0xC8, 0x89, 0x80, 0xC8, 0xA0, 0xB0, 0xE1, 0xFB, 0x12, 0x21, 0xC8, 0x88, 0x80,
    0xE8, 0x80, 0x80, 0x90, 0x09, 0xE8, 0x01, 0xA0, 0xC8, 0x80, 0x80, 0xBF, 0x2F, 0x81, 0xE8, 0x80,
    0x80, 0xC8, 0xF3, 0x81, 0xE1, 0x0C, 0x12, 0x21, 0x90, 0x25, 0xE9, 0xED, 0x81, 0xE8, 0x7B, 0x80,
    0x59, 0x49, 0x74, 0xE9, 0xF0, 0x81, 0xE2, 0x80, 0x2A, 0x73, 0x11, 0x2A, 0x7B, 0x99, 0x0A, 0xE8,
    0xF1, 0x81, 0x00, 0xC8, 0xF1, 0x81, 0xCC, 0x7B, 0x80, 0xE8, 0xEE, 0x81, 0xBC, 0xDA, 0x81, 0xE8,
    0xF2, 0x81, 0x90, 0x29, 0xE9, 0x7B, 0x80, 0xE8, 0xED, 0x81, 0x51, 0x72, 0xE8, 0xF1, 0x81, 0x98,
    0x16, 0x40, 0xC8, 0xF1, 0x81, 0x12, 0xE1, 0x80, 0x29, 0xE1, 0xB0, 0x79, 0x99, 0x04, 0x12, 0xBC,
    0xD4, 0x81, 0xE0, 0x30, 0xC8, 0x7B, 0x80, 0xE8, 0xEF, 0x81, 0xC8, 0xF2, 0x81, 0xE8, 0xF2, 0x81,
    0x90, 0x03, 0xBC, 0x24, 0x81, 0x40, 0xC8, 0xF2, 0x81, 0xBC, 0x24, 0x81, 0xB9, 0x01, 0x08, 0x0F,
    0xD0, 0x01, 0x01, 0x01
};

uint8_t disableplugin_value= 0xba;

int xdfpSetMem(libusb_device_handle *dev_handle, const uint8_t *buf, uint16_t length, uint16_t xdfpAddr) {
  const unsigned int kTimeout = 100;

  int ret = libusb_control_transfer(
    dev_handle,
    LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
    kMicronasSetMemReq,
    0,
    xdfpAddr,
    (unsigned char*)buf,
    length,
    kTimeout
  );
  if (ret < 0) {
    fprintf(stderr, "%s: libusb_control_transfer error %s\n", __func__, libusb_error_name(ret));
  }

  return 0;
}

int xdfpWrite(libusb_device_handle *dev_handle, uint16_t xdfpAddr, int32_t value) {
  uint8_t xdfpData[5];

  if (value < 0) value += 0x40000;
  xdfpData[0] = (value >> 10) & 0xff;
  xdfpData[1] = (value >> 2) & 0xff;
  xdfpData[2] = value & 0x03;
  xdfpData[3] = (xdfpAddr >> 8) & 0x03;
  xdfpData[4] = xdfpAddr & 0xff;

  return xdfpSetMem(dev_handle, xdfpData, 5, V8_WRITE_START_ADDR);
}

int downloadEQ(libusb_device_handle *dev_handle, enum TrinityAvailablePower availablePower) {
  const int32_t *eqSettings;

  switch (availablePower) {
  case POWER_4000MA:
    eqSettings = power4AEQSettings;
    break;
  case POWER_3000MA:
    eqSettings = power3AEQSettings;
    break;
  case POWER_1500MA:
    eqSettings = power1500mAEQSettings;
    break;
  default:
  case POWER_500MA:
    eqSettings = power500mAEQSettings;
    break;
  }
  
  uint16_t xdfpAddr;
  uint32_t eqIndex;
  int ret;
  for (eqIndex = 0, xdfpAddr = XDFP_STARTING_EQ_ADDR; eqIndex < EQ_TABLE_SIZE; eqIndex++, xdfpAddr++) {
    ret = xdfpWrite(dev_handle, xdfpAddr, eqSettings[eqIndex]);
    if (ret) {
      return ret;
    }
  }

  return ret;
}

int disablePlugin(libusb_device_handle *dev_handle) {
  return xdfpSetMem(dev_handle, &disableplugin_value, 1, V8_PLUGIN_START_ADDR);
}

int enablePlugin(libusb_device_handle *dev_handle) {
  return xdfpSetMem(dev_handle, pluginBinary, 1, V8_PLUGIN_START_ADDR);
}

int downloadPlugin(libusb_device_handle *dev_handle) {
  return xdfpSetMem(dev_handle, &pluginBinary[1], sizeof(pluginBinary), V8_PLUGIN_START_ADDR+1);
}

int main(int argc, char *argv[]) {
  /* Reading power delivery capacity */
  enum TrinityAvailablePower availablePower = POWER_NULL;
  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "--power-500") == 0) {
      printf("Audio device set to 500mA\n");
      availablePower = POWER_500MA;
    } else if (strcmp(argv[i], "--power-1500") == 0) {
      printf("Audio device set to 1500mA\n");
      availablePower = POWER_1500MA;
    } else if (strcmp(argv[i], "--power-3000") == 0) {
      printf("Audio device set to 3000mA\n");
      availablePower = POWER_3000MA;
    } else if (strcmp(argv[i], "--power-4000") == 0) {
      printf("Audio device set to 4000mA\n");
      availablePower = POWER_4000MA;
    }
  }

  if (availablePower == POWER_NULL) {
    printf("Available power settings :\n");
    printf("\t--power-500\t500mA\n");
    printf("\t--power-1500\t1500mA\n");
    printf("\t--power-3000\t3000mA\n");
    printf("\t--power-4000\t4000mA\n");
    
    return EXIT_SUCCESS;
  }

  libusb_context *usb_ctx;
  libusb_device_handle *dev_handle;

  /* Getting USB device interface */
  libusb_init(&usb_ctx);
  dev_handle = libusb_open_device_with_vid_pid(usb_ctx, 0x05AC, 0x1101);
  if (!dev_handle) {
    fprintf(stderr, "No suitable device found, exiting.\n");
    return EXIT_FAILURE;
  }
  
  if (disablePlugin(dev_handle)) {
    fprintf(stderr, "Error while disabling plugin.\n");
    goto usb_cleanup;
  }
  if (downloadEQ(dev_handle, availablePower)) {
    fprintf(stderr, "Error while downloading EQ to Trinity audio device.\n");
    goto usb_cleanup;
  }
  if (downloadPlugin(dev_handle)) {
    fprintf(stderr, "Error while downloading plugin to Trinity audio device.\n");
    goto usb_cleanup;
  }
  if (enablePlugin(dev_handle)) {
    fprintf(stderr, "Error while enabling plugin.\n");
    goto usb_cleanup;
  }

  libusb_close(dev_handle);
  libusb_exit(usb_ctx);

  return EXIT_SUCCESS;

usb_cleanup:
  libusb_close(dev_handle);
  libusb_release_interface(dev_handle, 0);
  libusb_exit(usb_ctx);
  return EXIT_FAILURE;
}
