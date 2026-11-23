//
// Created by urzu-7 on 9/1/26.
//

#include <Arduino.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include "Monitor.h"
#include "WifiManager.h"

bool WifiManager::start_ap = false;
bool WifiManager::disable_ap_fallback = false;
bool WifiManager::sta_connected = false;
const char *WifiManager::hostname = "esp32-unset-host";
IPAddress WifiManager::server_ip = IPAddress(192, 168, 4, 1);

void WifiManager::setupMdns() {
    if (!MDNS.begin(hostname)) {
        Serial.println("MDNS responder not available");
        return;
    }

    Serial.println("MDNS responder started: " + String(hostname) + ".local");
}

void WifiManager::addService(const char *serviceName, const char *serviceType, size_t port) {
    MDNS.addService(serviceName, serviceType, port);
}

bool WifiManager::connectToNetwork(const String &ssid, const String &password) {
    if (ssid.length() == 0) {
        return false;
    }

    // --- Connect to Wi-Fi ---
    Serial.print("📶 Connecting to Wi-Fi");
    WiFi.begin(ssid, password);
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n✅ WiFi connected!");
        server_ip = WiFi.localIP();
        Serial.print("📡 IP address: ");
        Serial.println(server_ip);
        setupMdns();
        sta_connected = true;
        return true;
    }
    Serial.println("\n❌ Failed to connect to Wi-Fi");
    Serial.println("SSID: " + ssid);
    Serial.println("PASS: " + password);
    WiFi.disconnect();
    sta_connected = false;
    return false;
}

bool WifiManager::startAccessPoint(const String &ap_ssid, const String &ap_password) {
    const auto gateway = IPAddress(192, 168, 4, 1);
    const auto subnet = IPAddress(255, 255, 255, 0);

    WiFi.softAPConfig(server_ip, gateway, subnet);

    if (WiFi.softAP(ap_ssid, ap_password, 1, 0, 4)) {
        Monitor::writeLine("AP started");
        Monitor::write("Network name: ");
        Monitor::writeLine(ap_ssid);
        Monitor::write("Password: ");
        Monitor::writeLine(ap_password);
        Monitor::write("IP address: ");
        Monitor::writeLine(server_ip.toString());
        setupMdns();
        return true;
    }
    Serial.println("AP failed to start");
    return false;
}

bool WifiManager::setup(const String &ap_ssid, const String &ap_password) {
    sta_connected = false;

    if (forced_ap) {
        if (disable_ap_fallback) {
            start_ap = false;
            return false;
        }
        start_ap = true;
        return startAccessPoint(ap_ssid, ap_password);
    }

    if (forced_sta) {
        start_ap = false;
        // 1. Try forced STA credentials
        if (forced_sta_ssid.length() > 0 && connectToNetwork(forced_sta_ssid, forced_sta_password)) {
            return true;
        }

        // 2. Fall back to NVS stored credentials
        prefs.begin("wifi", false);
        String nvs_ssid = prefs.getString("ssid", "");
        String nvs_pass = prefs.getString("pass", "");
        prefs.end();

        if (nvs_ssid.length() > 0 && connectToNetwork(nvs_ssid, nvs_pass)) {
            return true;
        }

        // 3. Fall back to default credentials
        if (default_sta_ssid.length() > 0 && connectToNetwork(default_sta_ssid, default_sta_password)) {
            return true;
        }

        // 4. Finally fall back to AP mode
        if (disable_ap_fallback) {
            return false;
        }
        start_ap = true;
        return startAccessPoint(ap_ssid, ap_password);
    }

    prefs.begin("wifi", false);
    start_ap = prefs.getBool("start_ap", false);
    String nvs_ssid = prefs.getString("ssid", "");
    String nvs_pass = prefs.getString("pass", "");
    prefs.end();

    if (disable_ap_fallback) {
        start_ap = false;
    }

    if (start_ap) {
        return startAccessPoint(ap_ssid, ap_password);
    }

    // When not on forced STA and start_ap is false:
    // 1. Attempt stored credentials from NVS if available
    if (nvs_ssid.length() > 0 && connectToNetwork(nvs_ssid, nvs_pass)) {
        return true;
    }

    // 2. Then default credentials if available
    if (default_sta_ssid.length() > 0 && connectToNetwork(default_sta_ssid, default_sta_password)) {
        return true;
    }

    // 3. Finally AP mode
    if (disable_ap_fallback) {
        return false;
    }
    start_ap = true;
    return startAccessPoint(ap_ssid, ap_password);
}

void WifiManager::stop(const uint32_t delay) {
    Monitor::writeLine("WiFi stopped");

    if (onWifiStopped) {
        onWifiStopped();
    }
    sta_connected = false;
    start_ap = false;
    disconnectDelay = delay;
    xTaskCreatePinnedToCore([](void *) {
        vTaskDelay(pdMS_TO_TICKS(disconnectDelay.load()));
        MDNS.end();
        WiFi.disconnect(true);
        WiFi.softAPdisconnect(true);
        WiFiClass::mode(WIFI_OFF);
    }, "WiFiStopTask", 4096, nullptr, 1, nullptr, 0);
}

String WifiManager::getSerializedState() {
    JsonDocument jsonDoc;
    jsonDoc["start_ap"] = start_ap;
    jsonDoc["sta_connected"] = sta_connected;
    jsonDoc["disable_ap_fallback"] = disable_ap_fallback;
    String state;
    serializeJson(jsonDoc, state);
    return state;
}

IPAddress WifiManager::getServerIP() {
    return server_ip;
}


void WifiManager::wifiConfigJsonCallback(const JsonDocument &doc) {
    prefs.begin("wifi", false);
    if (doc["start_ap"].is<bool>()) {
        start_ap = doc["start_ap"];
        if (disable_ap_fallback) {
            start_ap = false;
        }
        prefs.putBool("start_ap", start_ap);
        const auto mode = start_ap ? "AP" : "STA";
        Monitor::writeLine("WiFi starter mode set to: " + String(mode));
        if (start_ap) {
            prefs.end();
            return;
        }
    }

    if (!doc["ssid"].is<const char *>() || !doc["pass"].is<const char *>()) {
        // SerialQueue::enqueueLine("Missing 'ssid' or 'pass' in JSON payload");
        prefs.end();
        return;
    }

    const char *ssid = doc["ssid"];
    const char *pass = doc["pass"];

    if (strlen(ssid) == 0) {
        Monitor::writeLine("SSID is empty");
        prefs.end();
        return;
    }

    prefs.putString("ssid", ssid);
    prefs.putString("pass", pass);
    Monitor::writeLine("WiFi credentials overwritten");
    prefs.end();
}
