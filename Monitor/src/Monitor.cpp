//
// Created by urzu-7 on 8/31/26.
//
#include <Arduino.h>
#include "Monitor.h"

void Monitor::registerInstance(std::unique_ptr<IDebug> debugInstance) {
    debugInstances.push_back(std::move(debugInstance));
}

#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER

AsyncWebSocket &Monitor::enableAsyncWebSocketHandler(const char *path) {
    if (enabledAsyncWebSocketServer && ws)
        return *ws;

    enabledAsyncWebSocketServer = true;
    ws = std::make_unique<AsyncWebSocket>(path);
    ws->onEvent(onWsEvent);
    startWsTask();
    return *ws;
}

void Monitor::startWsTask() {
    if (!wsQueueMutex) {
        wsQueueMutex = xSemaphoreCreateMutex();
    }
    if (!wsTaskHandle) {
        xTaskCreate(wsTaskEntry, "MonitorWsTask", 8192, nullptr, 1, &wsTaskHandle);
    }
}

void Monitor::enqueueWsMessage(const std::string &message) {
    if (!wsQueueMutex) return;
    if (xSemaphoreTake(wsQueueMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        if (wsMessageQueue.size() < 50) {
            wsMessageQueue.push(message);
        }
        xSemaphoreGive(wsQueueMutex);
        if (wsTaskHandle) {
            xTaskNotifyGive(wsTaskHandle);
        }
    }
}

void Monitor::processWsQueue() {
    std::string msg;
    bool hasMsg = false;

    do {
        hasMsg = false;
        if (wsQueueMutex && xSemaphoreTake(wsQueueMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            if (!wsMessageQueue.empty()) {
                msg = std::move(wsMessageQueue.front());
                wsMessageQueue.pop();
                hasMsg = true;
            }
            xSemaphoreGive(wsQueueMutex);
        }

        if (hasMsg) {
            dispatchMessage(msg);
        }
    } while (hasMsg);
}

void Monitor::wsTaskEntry(void *) {
    while (true) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        processWsQueue();
    }
}

void Monitor::onWsEvent(AsyncWebSocket *, const AsyncWebSocketClient *client, AwsEventType type, void *arg,
                        const uint8_t *data, const size_t len) {
    if (!isInitialized.load())
        return;

    switch (type) {
        case WS_EVT_CONNECT:
            if (client) {
                writeLine("Ws client connected: " + client->remoteIP().toString());
            }
            break;
        case WS_EVT_DISCONNECT:
            if (client) {
                writeLine("Ws client disconnected: " + client->remoteIP().toString());
            }
            break;
        case WS_EVT_PING:
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        case WS_EVT_DATA: {
            if (const auto *info = static_cast<AwsFrameInfo *>(arg);
                info && info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
                const std::string rawMessage(reinterpret_cast<const char *>(data), len);
                enqueueWsMessage(rawMessage);
            }
            break;
        }
    }
}

#endif

void Monitor::dispatchMessage(const std::string &rawMsg) {
    String data = rawMsg.c_str();
    data.trim();
    if (data.isEmpty()) {
        return;
    }

    if (JsonDocument jsonDocument; !deserializeJson(jsonDocument, data)) {
        for (const auto &item: jsonCallbacks) {
            if (item) {
                item(jsonDocument);
            }
        }
    } else {
        for (const auto &item: callbacks) {
            if (item) {
                item(data.c_str());
            }
        }
    }
}

void Monitor::init() {
    for (const auto &instance: debugInstances) {
        instance->init();
    }

#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer) {
        startWsTask();
    }
#endif
    isInitialized = true;
}

void Monitor::write(const std::string &message) {
    for (const auto &instance: debugInstances) {
        instance->write(message);
    }

#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(message.c_str());
    }
#endif
}

void Monitor::write(const String &message) {
    for (const auto &instance: debugInstances) {
        instance->write(message);
    }
#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(message);
    }
#endif
}

void Monitor::write(const char *str) {
    for (const auto &instance: debugInstances) {
        instance->write(str);
    }
#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(str);
    }
#endif
}

void Monitor::writeLine(const std::string &message) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(message);
    }
#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(message.c_str());
    }
#endif
}

void Monitor::writeLine(const String &message) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(message);
    }
#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(message);
    }
#endif
}

void Monitor::writeLine(const char *str) {
    for (const auto &instance: debugInstances) {
        instance->writeLine(str);
    }
#ifdef ENABLE_USE_ASYNC_WEBSOCKET_SERVER
    if (enabledAsyncWebSocketServer && ws) {
        ws->textAll(str);
    }
#endif
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
