//
// Created by urzu-7 on 10/18/26.
//

#include "SerialManager.h"


SerialManager::SerialManager(HardwareSerial &uartPort, const int8_t rxPin, const int8_t txPin,
                             const uint32_t baudRate, const uint32_t period, const uint32_t bufferLength)
    : serialType(SerialType::HARDWARE_SERIAL), hwSerial(&uartPort), cdcSerial(nullptr), stream(&uartPort),
      rxPin(rxPin), txPin(txPin), baudRate(baudRate), period(period), bufferLength(bufferLength) {
}

SerialManager::SerialManager(HWCDC &serial, const uint32_t baudRate,
                             const uint32_t period, const uint32_t bufferLength)
    : serialType(SerialType::HW_CDC), hwSerial(nullptr), cdcSerial(&serial), stream(&serial),
      rxPin(-1), txPin(-1), baudRate(baudRate), period(period), bufferLength(bufferLength) {
}

SerialManager::~SerialManager() {
    SerialManager::stop();
    if (queueMutex) {
        vSemaphoreDelete(queueMutex);
        queueMutex = nullptr;
    }
}

void SerialManager::init() {
    if (isRunning()) {
        return;
    }
    if (serialType == SerialType::HARDWARE_SERIAL && hwSerial) {
        hwSerial->begin(baudRate, SERIAL_8N1, rxPin, txPin);
        hwSerial->setTimeout(1000);
    } else if (serialType == SerialType::HW_CDC && cdcSerial) {
        cdcSerial->begin(baudRate);
        cdcSerial->setTimeout(1000);
    }
    if (!queueMutex) {
        queueMutex = xSemaphoreCreateMutex();
    }
    running = true;
    if (!mainTask) {
        xTaskCreate(taskEntry, "SerialQueue", 8192, this, 1, &mainTask);
    }
}

void SerialManager::init(const uint32_t newBaudRate, const uint32_t newPeriod) {
    this->baudRate = newBaudRate;
    this->period = newPeriod;
    init();
}

void SerialManager::stop() {
    if (!running && !mainTask) {
        return;
    }
    running = false;

    if (mainTask != nullptr) {
        if (xTaskGetCurrentTaskHandle() != mainTask) {
            for (int i = 0; i < 20 && mainTask != nullptr; ++i) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            if (mainTask != nullptr) {
                const TaskHandle_t taskToDelete = mainTask;
                mainTask = nullptr;
                vTaskDelete(taskToDelete);
            }
        }
    }

    if (queueMutex && xSemaphoreTake(queueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        while (!serialQueue.empty()) {
            if (stream) {
                stream->print(serialQueue.front().c_str());
            }
            serialQueue.pop();
        }
        xSemaphoreGive(queueMutex);
    }
    if (serialType == SerialType::HARDWARE_SERIAL && hwSerial) {
        hwSerial->flush();
        hwSerial->end();
    } else if (serialType == SerialType::HW_CDC && cdcSerial) {
        cdcSerial->flush();
        cdcSerial->end();
    }
}


void SerialManager::run() {
    if (queueMutex && xSemaphoreTake(queueMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        while (!serialQueue.empty()) {
            if (stream) {
                stream->print(serialQueue.front().c_str());
            }
            serialQueue.pop();
        }
        xSemaphoreGive(queueMutex);
    }

    if (stream) {
        while (stream->available() > 0) {
            if (const char c = static_cast<char>(stream->read()); c == '\r' || c == '\n') {
                rxBuffer.trim();
                if (!rxBuffer.isEmpty()) {
                    const String data = rxBuffer;
                    rxBuffer = "";
                    if (JsonDocument jsonDocument; !deserializeJson(jsonDocument, data)) {
                       onJsonReceived(jsonDocument);
                    } else {
                       onCommandReceived(data.c_str());
                    }
                }
            } else {
                if (rxBuffer.length() < bufferLength) {
                    rxBuffer += c;
                }
            }
        }
    }
}


void SerialManager::write(const std::string &message) {
    if (!running || !queueMutex) return;
    if (xSemaphoreTake(queueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        if (serialQueue.size() < 100) {
            // Limit queue to prevent unbounded RAM usage
            serialQueue.push(message);
        }
        xSemaphoreGive(queueMutex);
    }
}

