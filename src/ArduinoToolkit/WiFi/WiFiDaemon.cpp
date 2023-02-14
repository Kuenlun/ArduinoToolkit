#include <vector>

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
#include <esp32-hal-log.h>

#include "WiFiDaemon.h"

namespace AT
{

    bool WiFiDaemon::s_taskCreatedFlag{false};
    const char *WiFiDaemon::s_wifiSSID{nullptr};
    const char *WiFiDaemon::s_wifiPASS{nullptr};
    SemaphoreHandle_t WiFiDaemon::s_binarySemphrWiFiConnected{nullptr};
    SemaphoreHandle_t WiFiDaemon::s_binarySemphrTryToConnectWiFi{nullptr};
    std::vector<TaskHandle_t> WiFiDaemon::s_wifiDependentTasks;

    void WiFiDaemon::timerReconnectWiFiCB(const TimerHandle_t xTimer)
    {
        if (!uxSemaphoreGetCount(s_binarySemphrWiFiConnected))
        {
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            xSemaphoreGiveFromISR(s_binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }
    }

    // Suspend all tasks included in "s_wifiDependentTasks" vector
    void WiFiDaemon::suspendWiFiDependentTasks()
    {
        if (s_wifiDependentTasks.size())
        {
            for (const TaskHandle_t &task : s_wifiDependentTasks)
            {
                vTaskSuspend(task);
                isr_log_v("Task %s suspended", pcTaskGetName(task));
            }
            isr_log_d("WiFi dependent tasks suspended");
        }
    }

    // Resume all tasks included in "s_wifiDependentTasks" vector
    void WiFiDaemon::resumeWiFiDependentTasks()
    {
        if (s_wifiDependentTasks.size())
        {
            for (const TaskHandle_t &task : s_wifiDependentTasks)
            {
                xTaskResumeFromISR(task);
                isr_log_v("Task %s resumed", pcTaskGetName(task));
            }
            isr_log_d("WiFi dependent tasks resumed");
        }
    }

    void WiFiDaemon::WiFiEventCB(const WiFiEvent_t &event, const WiFiEventInfo_t &info)
    {
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        switch (event)
        {
        // Got WIFI_STA_DISCONNECTED event (WiFi disconnected)
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
        {
            const uint8_t reason{info.wifi_sta_disconnected.reason};
            isr_log_w("ARDUINO_EVENT_WIFI_STA_DISCONNECTED (Reason: %u)", reason);
            // Take the "s_binarySemphrWiFiConnected" to notify to other tasks that WiFi is OFF
            xSemaphoreTakeFromISR(s_binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
            isr_log_v("s_binarySemphrWiFiConnected set to 0");
            // Suspend all tasks included in "s_wifiDependentTasks" vector
            suspendWiFiDependentTasks();
            if (!reason)
                log_e("WIFI_STA_DISCONNECTED with reason 0");
            // On reson ASSOC_FAIL wait some time to try to reconnect, otherwise reconnect immediately
            if (reason != WIFI_REASON_ASSOC_FAIL)
                xSemaphoreGiveFromISR(s_binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
            break;
        }
        // Got WIFI_STA_GOT_IP event (WiFi connection stablished)
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
        {
            isr_log_i("ARDUINO_EVENT_WIFI_STA_GOT_IP. WiFi connected");
            // Give the "s_binarySemphrWiFiConnected" to notify to other tasks that WiFi is ON
            xSemaphoreGiveFromISR(s_binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
            isr_log_v("s_binarySemphrWiFiConnected set to 1");
            // Resume all tasks included in "s_wifiDependentTasks" vector
            resumeWiFiDependentTasks();
            // Take the "s_binarySemphrTryToConnectWiFi" to stop reconnecting
            xSemaphoreTakeFromISR(s_binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
            break;
        }
        default:
            break;
        }

        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

    void WiFiDaemon::WiFiDaemonTask(void *const parameters)
    {
        // Create a binary semaphore to know when WiFi is connected
        s_binarySemphrWiFiConnected = xSemaphoreCreateBinary();
        if (!s_binarySemphrWiFiConnected)
            log_e("Could not create binary semaphore");

        // Create binary semaphore to know when to reconnect WiFi
        s_binarySemphrTryToConnectWiFi = xSemaphoreCreateBinary();
        if (!s_binarySemphrTryToConnectWiFi)
            log_e("Could not create binary semaphore");
        xSemaphoreGive(s_binarySemphrTryToConnectWiFi);

        // Create the WiFi connection timeout timer
        static TimerHandle_t timerReconnectWiFi{nullptr};
        timerReconnectWiFi = xTimerCreate(
            "timerReconnectWiFi",
            pdMS_TO_TICKS(s_WIFI_RECONNECT_WAIT_TIME_MS),
            pdTRUE,
            (void *)0,
            timerReconnectWiFiCB);
        if (!timerReconnectWiFi)
            log_e("Could not create timer");
        // Start the timer
        xTimerStart(timerReconnectWiFi, portMAX_DELAY);

        // Set the WiFi callback
        WiFi.onEvent(WiFiEventCB);
        // Disable auto reconnect, the current task will take care of that
        WiFi.setAutoReconnect(false);
        // Configure WiFi as STA
        WiFi.mode(WIFI_STA);

        // Connect or reconect to WiFi
        while (true)
        {
            if (xSemaphoreTake(s_binarySemphrTryToConnectWiFi, portMAX_DELAY))
            {
                log_w("Task %s memory left: %u", pcTaskGetName(nullptr), uxTaskGetStackHighWaterMark(nullptr));
                // Connect to WiFi
                log_d("Connecting to %s", s_wifiSSID);
                WiFi.begin(s_wifiSSID, s_wifiPASS);
            }
        }
    }

    WiFiDaemon::WiFiDaemon(const char *const ssid,
                           const char *const passphrase,
                           const UBaseType_t uxPriority)
    {
        setDaemonCreatedFlag(s_taskCreatedFlag);

        s_wifiSSID = ssid;
        s_wifiPASS = passphrase;

        xTaskCreatePinnedToCore(
            WiFiDaemonTask,
            "WiFiDaemonTask",
            3 * 1024,
            nullptr,
            uxPriority,
            &getTaskHandle(),
            ARDUINO_RUNNING_CORE);
    }

    WiFiDaemon::~WiFiDaemon()
    {
        s_taskCreatedFlag = false;
    }

    BaseType_t WiFiDaemon::blockUntilConnected(const TickType_t &xTicksToWait)
    {
        if (s_binarySemphrWiFiConnected)
            return xQueuePeek(s_binarySemphrWiFiConnected, (void *)nullptr, xTicksToWait);
        return pdFALSE;
    }

    bool WiFiDaemon::isConnected()
    {
        if (s_binarySemphrWiFiConnected)
            return uxSemaphoreGetCount(s_binarySemphrWiFiConnected);
        return false;
    }

    void WiFiDaemon::addDependentTask(TaskHandle_t task)
    {
        // If task is nullptr get the current task handle
        if (!task)
            task = xTaskGetCurrentTaskHandle();
        // Add the task to the "s_wifiDependentTasks" vector
        s_wifiDependentTasks.push_back(task);
        // Suspend the task if the WiFi is not enabled
        if (!isConnected())
        {
            log_v("Task %s suspended", pcTaskGetName(task));
            vTaskSuspend(task);
        }
    }

} // namespace AT
