//
// Created by urzu-7 on 11/10/26.
//

#ifndef ESPMANAGERS_IDEBUG_H
#define ESPMANAGERS_IDEBUG_H

#include <queue>
#include <string>
#include <vector>
#include <functional>
#include <EspEvent.h>
#include "ArduinoJson.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

class IDebug {
    void loop();

    virtual void run() = 0;

protected:
    std::queue<std::string> serialQueue;
    SemaphoreHandle_t queueMutex = nullptr;
    bool running = false;
    uint32_t period = 25;
    TaskHandle_t mainTask = nullptr;

    static void taskEntry(void *pv);

public:
    virtual ~IDebug() = default;

    virtual void init() = 0;

    virtual void stop() = 0;

    EspEvent<const char *> onCommandReceived;
    EspEvent<const JsonDocument &> onJsonReceived;

    [[nodiscard]] bool isRunning() const { return running; }

    virtual void write(const std::string &message) = 0;

    void writeLine(const std::string &message);

    void write(const String &message);

    void writeLine(const String &message);

    void write(const char *str);

    void writeLine(const char *str);
};


#endif //ESPMANAGERS_IDEBUG_H
