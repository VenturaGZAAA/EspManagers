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

    usb.onHIDInput([](const EspUsbHostHIDInput &input) {
        onRawHIDInputEvent(input);
        if (!forcedConsumerControlCorrection) {
            return;
        }
        if (EspUsbHostConsumerControlEvent event; forceMissIdentifiedConsumerEvent(input, event)) {
            onConsumerControlEvent(event);
        }
    });

    xTaskCreatePinnedToCore(reinterpret_cast<TaskFunction_t>(handleUSB), "usbTask", 4096, nullptr, 2, nullptr, 1);
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
            vTaskDelay(1500);
        }
    }
    vTaskDelete(nullptr);
}

bool UsbHIDManager::forceMissIdentifiedConsumerEvent(const EspUsbHostHIDInput &input,
                                                     EspUsbHostConsumerControlEvent &event) {
    if (input.data == nullptr || input.length < 3 || (
            input.data[0] != MINI_KEYBOARD_TOUCHPAD && input.data[0] != KITCHOP_GAMING_KEYBOARD && input.data[0] !=
            MACHENIKE_KEYBOARD)) {
        return false;
    }

    const uint16_t usage = static_cast<uint16_t>(input.data[1]) | (static_cast<uint16_t>(input.data[2]) << 8);
    static uint16_t previousUsages[32] = {};
    const uint8_t addrIdx = input.address < 32 ? input.address : 0;
    uint16_t &previousUsage = previousUsages[addrIdx];

    if (usage == previousUsage) {
        return false;
    }

    event = EspUsbHostConsumerControlEvent();
    event.address = input.address;
    event.interfaceNumber = input.interfaceNumber;
    event.vid = input.vid;
    event.pid = input.pid;
    event.manufacturer = input.manufacturer;
    event.product = input.product;
    event.serial = input.serial;
    event.rawData = input.data;
    event.rawLength = input.length;
    event.reportData = input.data + 1;
    event.reportLength = input.length - 1;
    event.usage = usage != 0 ? usage : previousUsage;
    event.pressed = usage != 0;
    event.released = (usage == 0 && previousUsage != 0);

    previousUsage = usage;
    return event.pressed || event.released;
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

const char *UsbHIDManager::className(const uint8_t cls) {
    switch (cls) {
        case 0x00:
            return "per-interface";
        case 0x01:
            return "Audio";
        case 0x02:
            return "CDC Control";
        case 0x03:
            return "HID";
        case 0x07:
            return "Printer"; // ESC/POS receipt printers, IPP/raw printers
        case 0x08:
            return "Mass Storage";
        case 0x09:
            return "Hub";
        case 0x0a:
            return "CDC Data";
        case 0x0b:
            return "Smart Card"; // CCID
        case 0x0e:
            return "Video";
        case 0xfe:
            return "Application Specific"; // USBTMC, DFU, IrDA bridge
        case 0xff:
            return "Vendor";
        default:
            return "Unknown";
    }
}

const char *UsbHIDManager::speedName(const usb_speed_t speed) {
    switch (speed) {
        case USB_SPEED_LOW:
            return "low-speed";
        case USB_SPEED_FULL:
            return "full-speed";
        case USB_SPEED_HIGH:
            return "high-speed";
        default:
            return "unknown";
    }
}

char UsbHIDManager::displayChar(const uint8_t ascii) {
    return (ascii >= 0x20 && ascii != 0x7F) ? static_cast<char>(ascii) : '.';
}

const char *UsbHIDManager::consumerControlUsageNaming(const uint16_t usage) {
    switch (usage) {
        case ESP_USB_HOST_CONSUMER_CONTROL_NEXT_TRACK:
            return "Next";
        case ESP_USB_HOST_CONSUMER_CONTROL_PREVIOUS_TRACK:
            return "Previous";
        case ESP_USB_HOST_CONSUMER_CONTROL_PLAY_PAUSE:
            return "Play/Pause";
        case ESP_USB_HOST_CONSUMER_CONTROL_MUTE:
            return "Mute";
        case ESP_USB_HOST_CONSUMER_CONTROL_VOLUME_UP:
            return "Volume Up";
        case ESP_USB_HOST_CONSUMER_CONTROL_VOLUME_DOWN:
            return "Volume Down";
        case ESP_USB_HOST_CONSUMER_CONTROL_EMAIL:
            return "Email";
        case ESP_USB_HOST_CONSUMER_CONTROL_MEDIA:
            return "Media";
        case ESP_USB_HOST_CONSUMER_CONTROL_SEARCH:
            return "Search";
        case ESP_USB_HOST_CONSUMER_CONTROL_HOME:
            return "Home";
        case ESP_USB_HOST_CONSUMER_CONTROL_E:
            return "E";
        default:
            return "";
    }
}

String UsbHIDManager::getHexHelper(const uint8_t *data, const size_t length) {
    String output = "";
    for (size_t i = 0; i < length; i++) {
        if (data[i] < 0x10) {
            output += '0';
        }
        output += String(data[i], HEX);
        if (i + 1 < length) {
            output += ' ';
        }
    }
    return output;
}

JsonDocument UsbHIDManager::serializeDeviceConnection(const EspUsbHostDeviceInfo &deviceInfo, const char *type) {
    JsonDocument doc;
    const JsonObject connection = doc["connection"].to<JsonObject>();
    connection["type"] = type;
    connection["address"] = deviceInfo.address;
    connection["portId"] = deviceInfo.portId;
    connection["vid"] = deviceInfo.vid;
    connection["pid"] = deviceInfo.pid;
    connection["deviceClass"] = className(deviceInfo.deviceClass);
    connection["speed"] = speedName(deviceInfo.speed);
    connection["product"] = deviceInfo.product;
    return doc;
}

JsonDocument UsbHIDManager::serializeRawHID(const EspUsbHostHIDInput &input) {
    JsonDocument doc;
    const JsonObject rawEvent = doc["rawHID"].to<JsonObject>();
    rawEvent["address"] = input.address;
    rawEvent["vid"] = input.vid;
    rawEvent["pid"] = input.pid;
    rawEvent["interfaceNumber"] = input.interfaceNumber;
    rawEvent["subclass"] = input.subclass;
    rawEvent["protocol"] = input.protocol;
    rawEvent["length"] = input.length;
    rawEvent["data"] = getHexHelper(input.data, input.length);

    return doc;
}

JsonDocument UsbHIDManager::serializeKeyboard(const EspUsbHostKeyboardEvent &event) {
    JsonDocument doc;
    const JsonObject keyboardEvent = doc["keyboardEvent"].to<JsonObject>();
    keyboardEvent["type"] = event.pressed ? "press" : "release";
    keyboardEvent["keycode"] = event.keycode;
    keyboardEvent["ascii"] = event.ascii;
    keyboardEvent["display"] = String(displayChar(event.ascii));
    keyboardEvent["modifiers"] = getModifiers(event.modifiers);
    return doc;
}

JsonDocument UsbHIDManager::serializeMouseEvent(const EspUsbHostMouseEvent &event) {
    JsonDocument doc;
    const JsonObject mouseEvent = doc["mouseEvent"].to<JsonObject>();
    mouseEvent["buttons"] = event.buttons;
    mouseEvent["x"] = event.x;
    mouseEvent["y"] = event.y;
    mouseEvent["wheel"] = event.wheel;
    mouseEvent["pan"] = event.pan;
    mouseEvent["buttonsChanged"] = event.buttonsChanged;
    mouseEvent["moved"] = event.moved;
    return doc;
}

JsonDocument UsbHIDManager::seriializeConsumerControlEvent(const EspUsbHostConsumerControlEvent &event) {
    JsonDocument doc;
    const JsonObject consumerControlEvent = doc["consumerControlEvent"].to<JsonObject>();
    consumerControlEvent["type"] = event.pressed ? "press" : "release";
    consumerControlEvent["usage"] = event.usage;
    consumerControlEvent["name"] = consumerControlUsageNaming(event.usage);

    return doc;
}
