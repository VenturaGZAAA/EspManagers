//
// Created by urzu-7 on 11/10/26.
//

#ifndef ESPMANAGERS_USBHIDMANAGER_H
#define ESPMANAGERS_USBHIDMANAGER_H

#include "EspUsbHost.h"
#include "EspEvent.h"
#include "ArduinoJson.h"

class UsbHIDManager {
    static inline EspUsbHost usb;
    static inline bool usbStarted = false;
    static inline EspUsbHostKeyboardLayout layout = ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US;
    static void handleUSB();

public:
    static void setKeyboardLayout(EspUsbHostKeyboardLayout _layout) {
        layout = _layout;
    };

    static String getModifiers(const uint8_t modifiers);

    static inline EspEvent<EspUsbHostDeviceInfo> onDeviceConnected;

    static inline EspEvent<EspUsbHostKeyboardEvent> onKeyboardEvent;
    static inline EspEvent<JsonDocument> onKeyboardJsonEvent;
    static inline EspEvent<EspUsbHostMouseEvent> onMouseEvent;
    static inline EspEvent<JsonDocument> onMouseJsonEvent;
    static inline EspEvent<EspUsbHostConsumerControlEvent> onConsumerControlEvent;
    static inline EspEvent<JsonDocument> onConsumerControlJsonEvent;

    static void init();
};


#endif //ESPMANAGERS_USBHIDMANAGER_H
