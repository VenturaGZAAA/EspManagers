//
// Created by urzu-7 on 11/10/26.
//

#include "IDebug.h"


void IDebug::taskEntry(void *pv) {
    auto *task = static_cast<IDebug *>(pv);
    task->loop();
    task->mainTask = nullptr;
    vTaskDelete(nullptr);
}

void IDebug::loop() {
    const TickType_t xFrequency = pdMS_TO_TICKS(period); // 20 ms period (50 Hz)
    TickType_t xLastWakeTime = xTaskGetTickCount();

    while (running) {
        run();
        vTaskDelayUntil(&xLastWakeTime, xFrequency);
    }
}

void IDebug::write(const String &message) {
    write(std::string(message.c_str()));
}

void IDebug::write(const char *str) {
    if (str) {
        write(std::string(str));
    }
}

void IDebug::writeLine(const std::string &message) {
    write(message + "\r\n");
}

void IDebug::writeLine(const String &message) {
    write(message + "\r\n");
}

void IDebug::writeLine(const char *str) {
    write(str + String("\r\n"));
}
