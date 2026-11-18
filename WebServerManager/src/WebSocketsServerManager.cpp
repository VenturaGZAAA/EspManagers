//
// Created by urzu-7 on 11/18/26.
//

#include "WebSocketsServerManager.h"

WebSocketsServerManager::~WebSocketsServerManager() {
    WebSocketsServerManager::stop();
    if (wsQueueMutex) {
        vSemaphoreDelete(wsQueueMutex);
        wsQueueMutex = nullptr;
    }
}

void WebSocketsServerManager::init() {
    if (isRunning()) {
        return;
    }
    running = true;
    startWsTask();
}

void WebSocketsServerManager::stop() {
    if (!running && !wsTaskHandle) {
        return;
    }
    running = false;

    if (wsTaskHandle != nullptr) {
        xTaskNotifyGive(wsTaskHandle);
        if (xTaskGetCurrentTaskHandle() != wsTaskHandle) {
            for (int i = 0; i < 20 && wsTaskHandle != nullptr; ++i) {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
            if (wsTaskHandle != nullptr) {
                TaskHandle_t taskToDelete = wsTaskHandle;
                wsTaskHandle = nullptr;
                vTaskDelete(taskToDelete);
            }
        }
    }

    if (wsQueueMutex && xSemaphoreTake(wsQueueMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        while (!wsMessageQueue.empty()) {
            wsMessageQueue.pop();
        }
        xSemaphoreGive(wsQueueMutex);
    }
}

void WebSocketsServerManager::run() {
    processWsQueue();
}

void WebSocketsServerManager::write(const std::string &message) {
    if (message.empty()) {
        return;
    }
    txBuffer += message;
    if (message.back() == '\n' || message.back() == '\r') {
        if (ws) {
            ws->textAll(txBuffer.c_str());
        }
        txBuffer.clear();
    }
}

AsyncWebSocket &WebSocketsServerManager::getWs(const String &wsPath) {
    if (ws) return *ws;
    path = wsPath;
    ws = std::make_unique<AsyncWebSocket>(path);
    ws->onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                       void *arg, uint8_t *data, size_t len) {
        onWsEvent(server, client, type, arg, data, len);
    });
    startWsTask();
    return *ws;
}

void WebSocketsServerManager::startWsTask() {
    if (!wsQueueMutex) {
        wsQueueMutex = xSemaphoreCreateMutex();
    }
    if (!wsTaskHandle) {
        xTaskCreate(wsTaskEntry, "WsServerTask", 8192, this, 1, &wsTaskHandle);
    }
}

void WebSocketsServerManager::wsTaskEntry(void *pv) {
    auto *self = static_cast<WebSocketsServerManager *>(pv);
    while (self->running) {
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(50));
        self->processWsQueue();
    }
    self->wsTaskHandle = nullptr;
    vTaskDelete(nullptr);
}

void WebSocketsServerManager::enqueueWsMessage(const std::string &message) {
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

void WebSocketsServerManager::processWsQueue() {
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
            String data = msg.c_str();
            data.trim();
            if (data.isEmpty()) {
                continue;
            }

            if (JsonDocument jsonDocument; !deserializeJson(jsonDocument, data)) {
                onJsonReceived(jsonDocument);
            } else {
                onCommandReceived(data.c_str());
            }
        }
    } while (hasMsg);
}

void WebSocketsServerManager::onWsEvent(AsyncWebSocket *, AsyncWebSocketClient *client, AwsEventType type,
                                        void *arg, uint8_t *data, size_t len) {
    if (!running)
        return;

    switch (type) {
        case WS_EVT_CONNECT:
            if (client) {
                writeLine(String("Ws client connected: ") + client->remoteIP().toString());
            }
            break;
        case WS_EVT_DISCONNECT:
            if (client) {
                writeLine(String("Ws client disconnected: ") + client->remoteIP().toString());
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
