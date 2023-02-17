#pragma once

#include <Arduino.h>
#include <WiFi.h>

#include "Daemon.h"

namespace AT
{

    class WiFiDaemon : public Daemon<WiFiDaemon>
    {
    public:
        static BaseType_t blockUntilConnected(const TickType_t xTicksToWait = portMAX_DELAY);
        static bool isConnected();
        static void addDependentTask(TaskHandle_t task = nullptr);

    protected:
        WiFiDaemon(const char *const ssid,
                   const char *const passphrase,
                   const UBaseType_t uxPriority);
        ~WiFiDaemon();

    private:
        static void timerReconnectWiFiCB(const TimerHandle_t xTimer);
        static void suspendWiFiDependentTasks();
        static void resumeWiFiDependentTasks();
        static void WiFiEventCB(const WiFiEvent_t &event, const WiFiEventInfo_t &info);
        static void WiFiDaemonTask(void *const parameters);

    private:
        TaskHandle_t m_taskHandle;

    private:
        static constexpr uint32_t s_WIFI_RECONNECT_WAIT_TIME_MS{5 * 1000};
        static const char *s_wifiSSID;
        static const char *s_wifiPASS;
        static SemaphoreHandle_t s_binarySemphrWiFiConnected;
        static SemaphoreHandle_t s_binarySemphrTryToConnectWiFi;
        static std::vector<TaskHandle_t> s_wifiDependentTasks;
    }; // class WiFiDaemon

} // namespace AT
