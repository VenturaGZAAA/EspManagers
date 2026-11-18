//
// Created by urzu-7 on 8/31/26.
//

#ifndef ENLATADORA_IOT_MONITOR_H
#define ENLATADORA_IOT_MONITOR_H

#include <IDebug.h>
#include <string>
#include <functional>
#include <atomic>
#include <memory>
#include <vector>
#include <ArduinoJson.h>

class Monitor {
    class MonitorPrint : public Print {
    public:
        size_t write(const uint8_t c) override {
            const char buf[2] = {static_cast<char>(c), '\0'};
            Monitor::write(buf);
            return 1;
        }
        size_t write(const uint8_t *buffer, const size_t size) override {
            if (buffer && size > 0) {
                const std::string s(reinterpret_cast<const char *>(buffer), size);
                Monitor::write(s);
                return size;
            }
            return 0;
        }
    };

    static inline std::vector<std::unique_ptr<IDebug>> debugInstances;
    static inline std::vector<std::function<void(const char *)>> callbacks;
    static inline std::vector<std::function<void(JsonDocument)>> jsonCallbacks;

    static inline std::atomic<bool> isInitialized{false};

public:
    Monitor() = delete;
    static inline MonitorPrint print;

    static void write(const std::string &message);
    static void writeLine(const std::string &message);

    static void write(const String &message);
    static void writeLine(const String &message);

    static void write(const char *str);
    static void writeLine(const char *str);

    static void registerCallback(const std::function<void(const char *)> &callback);
    static void registerJsonCallback(const std::function<void(JsonDocument)> &callback);

    static void registerInstance(std::unique_ptr<IDebug> debugInstance);

    static void init();
};

#endif // ENLATADORA_IOT_MONITOR_H
