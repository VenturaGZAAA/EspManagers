//
// Created by urzu-7 on 11/10/26.
//

#include "UsbHIDManager.h"

void UsbHIDManager::init() {
    usb.setKeyboardLayout(layout);
    usb.setHubTrackingEnabled(false);

    usb.onDeviceConnected([](const EspUsbHostDeviceInfo &info) {
        onDeviceConnected(info);
    });
    usb.onKeyboard([](const EspUsbHostKeyboardEvent &event) {
        onKeyboardEvent(event);
    });
    usb.onMouse([](const EspUsbHostMouseEvent &event) {
        onMouseEvent(event);
    });
    usb.onConsumerControl([](const EspUsbHostConsumerControlEvent &event) {
        onConsumerControlEvent(event);
    });

    xTaskCreatePinnedToCore(reinterpret_cast<TaskFunction_t>(handleUSB), "usbTask", 4096, nullptr, 2, nullptr, 1);

}

String UsbHIDManager::getModifiers(const uint8_t modifiers) {
    String msg = "";
    static const char *modifierNames[] = {
        "LCTRL", "LSHIFT", "LALT", "LGUI",
        "RCTRL", "RSHIFT", "RALT", "RGUI"
    };

    if (modifiers == 0) {
        msg = "none";
        return msg;
    }

    bool first = true;
    for (int i = 0; i < 8; i++) {
        if (modifiers & (1 << i)) {
            if (!first) {
                msg += "+";
            }
            msg += modifierNames[i];
            first = false;
        }
    }
    return msg;
}

void UsbHIDManager::handleUSB() {
    while (!usbStarted) {
        usbStarted = usb.begin();
        if (!usbStarted) {
            // JsonDocument doc;
            // doc["usbFailure"] = usb.lastErrorName();
            // String output;
            // serializeJson(doc, output);
            // Monitor::writeLine(output);
        }
        vTaskDelay(2500);
    }
    vTaskDelete(nullptr);
}
