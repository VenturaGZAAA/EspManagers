//
// Created by urzu-7 on 8/31/26.
//
#include <Arduino.h>
#include "Monitor.h"

void Monitor::registerInstance(std::unique_ptr<IDebug> debugInstance) {
    for (const auto &callback: callbacks) {
        debugInstance->onCommandReceived += callback;
    }
    for (const auto &jsonCallback: jsonCallbacks) {
        debugInstance->onJsonReceived += jsonCallback;
    }
    if (isInitialized.load()) {
        debugInstance->init();
    }
    debugInstances.push_back(std::move(debugInstance));
}

void Monitor::init() {
    for (const auto &instance: debugInstances) {
        instance->init();
    }
    isInitialized = true;
}

void Monitor::write(const std::string &message) {
    for (const auto &instance: debugInstances) {
        instance->write(message);
    }
}

void Monitor::write(const String &message) {
    for (const auto &instance: debugInstances) {
        instance->write(message);
    }
}

void Monitor::write(const char *str) {
    for (const auto &instance: debugInstances) {
        instance->write(str);
    }
}

void Monitor::writeLine(const std::string &message) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(message);
    }
}

void Monitor::writeLine(const String &message) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(message);
    }
}

void Monitor::writeLine(const char *str) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(str);
    }
}

void Monitor::registerCallback(const std::function<void(const char *)> &callback) {
    callbacks.push_back(callback);
    for (const auto &instance: debugInstances) {
        instance->onCommandReceived += callback;
    }
}

void Monitor::registerJsonCallback(const std::function<void(JsonDocument)> &callback) {
    jsonCallbacks.push_back(callback);
    for (const auto &instance: debugInstances) {
        instance->onJsonReceived += callback;
    }
}
