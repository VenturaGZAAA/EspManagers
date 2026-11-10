//
// Created by urzu-7 on 10/18/26.
//

#ifndef ENLATADORA_CONTROL_UARTMANAGER_H
#define ENLATADORA_CONTROL_UARTMANAGER_H

#include <IDebug.h>
#include <string>
#include <memory>
#include <Arduino.h>
#include <HardwareSerial.h>
#include <HWCDC.h>


class SerialManager :public IDebug {
    enum class SerialType {
        HARDWARE_SERIAL,
        HW_CDC
    };

    SerialType serialType;
    HardwareSerial *hwSerial = nullptr;
    HWCDC *cdcSerial = nullptr;
    Stream *stream = nullptr;
    int8_t rxPin;
    int8_t txPin;
    uint32_t baudRate;
    uint32_t period;
    uint32_t bufferLength;
    // std::queue<std::string> serialQueue;
    // std::vector<std::function<void(const char *)> > serialCallbacks;
    // std::vector<std::function<void(JsonDocument)> > serialJsonCallbacks;

    // SemaphoreHandle_t queueMutex = nullptr;
    String rxBuffer = "";

    void run() override;

    //* THE CONSTRUCTORS ARE PRIVATE TO ENFORCE USAGE THROUGH UNIQUE POINTER CREATION
    explicit SerialManager(HardwareSerial &uartPort, int8_t rxPin = -1, int8_t txPin = -1, uint32_t baudRate = 115200,
                         uint32_t period = 25, uint32_t bufferLength = 1024);

    explicit SerialManager(HWCDC &serial, uint32_t baudRate = 115200,
                      uint32_t period = 25, uint32_t bufferLength = 1024);
    // THE CONSTRUCTORS ARE PRIVATE TO ENFORCE USAGE THROUGH UNIQUE POINTER CREATION *//
public:

    ~SerialManager() override;

    static std::unique_ptr<SerialManager> reserve() { return {}; }

    static std::unique_ptr<SerialManager> create(HardwareSerial &uartPort, const int8_t rxPin = -1, const int8_t txPin = -1, const uint32_t baudRate = 115200,
               const uint32_t period = 25, const uint32_t bufferLength = 1024) {
        return std::unique_ptr<SerialManager>(new SerialManager(uartPort, rxPin, txPin, baudRate, period, bufferLength));
    }

    static std::unique_ptr<SerialManager> createHWCDC(HWCDC &serial, const uint32_t baudRate = 115200,
               const uint32_t period = 25, const uint32_t bufferLength = 1024) {
        return std::unique_ptr<SerialManager>(new SerialManager(serial, baudRate, period, bufferLength));
    }

    void init() override;

    void init(uint32_t newBaudRate, uint32_t newPeriod = 25);

    void stop() override;

    void write(const std::string &message) override;

    // void registerCallback(const std::function<void(const char *)> &callback);
    //
    // void registerJsonCallback(const std::function<void(JsonDocument)> &callback);
};


#endif //ENLATADORA_CONTROL_UARTMANAGER_H
