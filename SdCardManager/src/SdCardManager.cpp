//
// Created by urzu-7 on 10/17/26.
//

#include "SdCardManager.h"
#include "Monitor.h"
#include <SPI.h>

void SdCardManager::ensureMutex() {
    if (!sdMutex) {
        sdMutex = xSemaphoreCreateRecursiveMutex();
    }
}

bool SdCardManager::lock(const TickType_t timeout) {
    ensureMutex();
    if (!sdMutex) {
        return false;
    }
    return xSemaphoreTakeRecursive(sdMutex, timeout) == pdTRUE;
}

void SdCardManager::unlock() {
    if (sdMutex) {
        xSemaphoreGiveRecursive(sdMutex);
    }
}

SemaphoreHandle_t SdCardManager::getMutex() {
    ensureMutex();
    return sdMutex;
}

bool SdCardManager::isMounted() {
    LockGuard lock(pdMS_TO_TICKS(100));
    return mounted;
}

const String &SdCardManager::getRootPath() {
    return rootPath;
}


void SdCardManager::spiInit(const SpiPins &pins) {
    SPI.begin(pins.sck, pins.miso, pins.mosi, pins.cs);
}

void SdCardManager::init(const SdSpiConfig &cfg,const String &_rootPath) {
    LockGuard lock(pdMS_TO_TICKS(5000));
    rootPath = _rootPath;

    if (!sd.begin(cfg)) {
        Monitor::writeLine(" ❌ Card Mount Failed!");
        SPI.end();
        mounted = false;
    } else {
        mounted = true;
        Monitor::writeLine("✅ SD card mounted");
    }
}

void SdCardManager::init(const SpiPins &pins, const SdSpiConfig &cfg, const String &_rootPath) {
    spiInit(pins);
    init(cfg, _rootPath);
}

String SdCardManager::resolvePath(const String &path) {
    LockGuard lock(pdMS_TO_TICKS(100));
    if (rootPath.isEmpty() || rootPath == "/") {
        if (path.startsWith("/")) {
            return path;
        }
        return "/" + path;
    }

    String cleanRoot = rootPath;
    while (cleanRoot.endsWith("/")) {
        cleanRoot = cleanRoot.substring(0, cleanRoot.length() - 1);
    }
    if (!cleanRoot.startsWith("/")) {
        cleanRoot = "/" + cleanRoot;
    }

    if (path.startsWith(cleanRoot)) {
        return path;
    }

    if (path.startsWith("/")) {
        return cleanRoot + path;
    }
    return cleanRoot + "/" + path;
}

bool SdCardManager::exists(const String &path) {
    if (const LockGuard lock(pdMS_TO_TICKS(1000)); !lock || !mounted) {
        return false;
    }
    const String fullPath = resolvePath(path);
    return sd.exists(fullPath.c_str());
}

bool SdCardManager::exists(const char *path) {
    if (!path) {
        return false;
    }
    return exists(String(path));
}

SdFile *SdCardManager::openFile(const String &path, oflag_t oflag) {
    if (const LockGuard lock(pdMS_TO_TICKS(1000)); !lock || !mounted) {
        return nullptr;
    }
    const String fullPath = resolvePath(path);
    auto *file = new SdFile();
    if (!file->open(&sd, fullPath.c_str(), oflag)) {
        delete file;
        return nullptr;
    }
    return file;
}

SdFile *SdCardManager::openFile(const char *path, const oflag_t oflag) {
    if (!path) {
        return nullptr;
    }
    return openFile(String(path), oflag);
}

bool SdCardManager::open(SdFile &file, const String &path, const oflag_t oflag) {
    if (const LockGuard lock(pdMS_TO_TICKS(1000)); !lock || !mounted) {
        return false;
    }
    const String fullPath = resolvePath(path);
    return file.open(&sd, fullPath.c_str(), oflag);
}

bool SdCardManager::open(SdFile &file, const char *path, const oflag_t oflag) {
    if (!path) {
        return false;
    }
    return open(file, String(path), oflag);
}

bool SdCardManager::closeFile(SdFile *&file) {
    if (!file) {
        return false;
    }
    if (const LockGuard lock(pdMS_TO_TICKS(1000)); !lock) {
        return false;
    }
    const bool res = file->close();
    delete file;
    file = nullptr;
    return res;
}

bool SdCardManager::close(SdFile &file) {
    if (const LockGuard lock(pdMS_TO_TICKS(1000)); !lock) {
        return false;
    }
    return file.close();
}