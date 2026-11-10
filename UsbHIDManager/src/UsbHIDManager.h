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
    static inline bool forcedConsumerControlCorrection = false;
    static inline EspUsbHostKeyboardLayout layout = ESP_USB_HOST_KEYBOARD_LAYOUT_EN_US;
    static constexpr uint16_t ESP_USB_HOST_CONSUMER_CONTROL_EMAIL = 394;
    static constexpr uint16_t ESP_USB_HOST_CONSUMER_CONTROL_MEDIA = 387;
    static constexpr uint16_t ESP_USB_HOST_CONSUMER_CONTROL_SEARCH = 545;
    static constexpr uint16_t ESP_USB_HOST_CONSUMER_CONTROL_HOME = 547;
    static constexpr uint16_t ESP_USB_HOST_CONSUMER_CONTROL_E = 406;

    /* TODO:
     *  Turn these into a dynamic vector given by the main setup
     */
    static constexpr uint8_t MINI_KEYBOARD_TOUCHPAD = 0x01;
    static constexpr uint8_t MACHENIKE_KEYBOARD = 0x03;
    static constexpr uint8_t KITCHOP_GAMING_KEYBOARD = 0x05;


    static void handleUSB();

    static String getHexHelper(const uint8_t *data, const size_t length);

    static  bool forceMissIdentifiedConsumerEvent(const EspUsbHostHIDInput &input, EspUsbHostConsumerControlEvent &event);

public:
    static void setKeyboardLayout(EspUsbHostKeyboardLayout _layout) {
        layout = _layout;
    };

    static void forceConsumerControlCorrection() {
        forcedConsumerControlCorrection = true;
    }

    static String getModifiers(uint8_t modifiers);

    static const char *className(uint8_t cls);

    static const char *speedName(usb_speed_t speed);

    static char displayChar(uint8_t ascii);

    static const char *consumerControlUsageNaming(const uint16_t usage);

    static inline EspEvent<EspUsbHostDeviceInfo> onDeviceConnected;
    static inline EspEvent<EspUsbHostDeviceInfo> onDeviceDisconnected;

    static inline EspEvent<EspUsbHostHIDInput> onRawHIDInputEvent;
    static inline EspEvent<EspUsbHostKeyboardEvent> onKeyboardEvent;
    static inline EspEvent<EspUsbHostMouseEvent> onMouseEvent;
    static inline EspEvent<EspUsbHostConsumerControlEvent> onConsumerControlEvent;

    static JsonDocument serializeDeviceConnection(const EspUsbHostDeviceInfo &deviceInfo, const char *type);

    static JsonDocument serializeRawHID(const EspUsbHostHIDInput &input);

    static JsonDocument serializeKeyboard(const EspUsbHostKeyboardEvent &event);

    static JsonDocument serializeMouseEvent(const EspUsbHostMouseEvent &event);
    static JsonDocument seriializeConsumerControlEvent(const EspUsbHostConsumerControlEvent &event);

    static void init();
};


#endif //ESPMANAGERS_USBHIDMANAGER_H
