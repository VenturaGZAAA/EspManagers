//
// Created by urzu-7 on 11/18/26.
//

#ifndef ESPMANAGERS_WEBSOCKETSSERVERMANAGER_H
#define ESPMANAGERS_WEBSOCKETSSERVERMANAGER_H

#include <Arduino.h>
#include <EspEvent.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <functional>
#include <utility>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <IDebug.h>

class WebSocketsServerManager: public IDebug {
    static WebSocketsServerManager instance;

    std::unique_ptr<AsyncWebSocket> ws = nullptr;

    std::string txBuffer;

    void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                   void *arg, uint8_t *data, size_t len);

    TaskHandle_t wsTaskHandle = nullptr;
    SemaphoreHandle_t wsQueueMutex = nullptr;
    std::queue<std::string> wsMessageQueue;
    String path = "/ws";

    static void wsTaskEntry(void *pv);
    void startWsTask();
    void enqueueWsMessage(const std::string &message);
    void processWsQueue();

    void run() override;

    WebSocketsServerManager() = default;

public:
    ~WebSocketsServerManager() override;

    WebSocketsServerManager(const WebSocketsServerManager &) = delete;
    WebSocketsServerManager &operator=(const WebSocketsServerManager &) = delete;
    WebSocketsServerManager(WebSocketsServerManager &&) = delete;
    WebSocketsServerManager &operator=(WebSocketsServerManager &&) = delete;

    static WebSocketsServerManager &getInstance() {
        return instance;
    }

    static std::unique_ptr<WebSocketsServerManager> reserve() {
        return std::unique_ptr<WebSocketsServerManager>(&instance);
    }

    static std::unique_ptr<WebSocketsServerManager> create() {
        return std::unique_ptr<WebSocketsServerManager>(&instance);
    }

    struct NoOpDeleter {
        void operator()(WebSocketsServerManager *) const {}
    };

    void init() override;
    void stop() override;

    void write(const std::string &message) override;

    AsyncWebSocket &getWs(const String &wsPath = "/ws");
};

#endif //ESPMANAGERS_WEBSOCKETSSERVERMANAGER_H
