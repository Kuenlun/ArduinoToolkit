#include <vector>

#include "ArduinoToolkit/WiFi/WiFiDaemon.h"

namespace AT
{

    namespace WiFiDaemon
    {

        /**
         * Static variables
         */
        static constexpr uint32_t WIFI_RECONNECT_WAIT_TIME_MS{5 * 1000};
        // WiFi credentials
        static const char *wifiSSID{nullptr};
        static const char *wifiPASS{nullptr};
        static TaskHandle_t taskHandle{nullptr};
        // FreeRTOS binary semaphore
        static SemaphoreHandle_t binarySemphrTryToConnectWiFi{nullptr};
        static SemaphoreHandle_t binarySemphrWiFiConnected{nullptr};
        // Vector of FreeRTOS tasks that rely on WiFi
        static std::vector<TaskHandle_t> wifiDependentTasks;

        /**
         * Static functions
         */
        static void timerReconnectWiFiCB(const TimerHandle_t xTimer)
        {
            if (!uxSemaphoreGetCount(binarySemphrWiFiConnected))
            {
                BaseType_t xHigherPriorityTaskWoken{pdFALSE};
                xSemaphoreGiveFromISR(binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
                // Did this action unblock a higher priority task?
                if (xHigherPriorityTaskWoken)
                    portYIELD_FROM_ISR();
            }
        }

        // Suspend all tasks included in "wifiDependentTasks" vector
        static void suspendWiFiDependentTasks()
        {
            if (wifiDependentTasks.size())
            {
                for (const TaskHandle_t &task : wifiDependentTasks)
                {
                    vTaskSuspend(task);
                    AT_LOG_V("Task %s suspended", pcTaskGetName(task));
                }
                AT_LOG_D("WiFi dependent tasks suspended");
            }
        }

        // Resume all tasks included in "wifiDependentTasks" vector
        static void resumeWiFiDependentTasks()
        {
            if (wifiDependentTasks.size())
            {
                for (const TaskHandle_t &task : wifiDependentTasks)
                {
                    xTaskResumeFromISR(task);
                    AT_LOG_V("Task %s resumed", pcTaskGetName(task));
                }
                AT_LOG_D("WiFi dependent tasks resumed");
            }
        }

        static void WiFiEventCB(const WiFiEvent_t &event, const WiFiEventInfo_t &info)
        {
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            switch (event)
            {
            // Got WIFI_STA_DISCONNECTED event (WiFi disconnected)
            case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            {
                const uint8_t reason{info.wifi_sta_disconnected.reason};
                AT_LOG_W("ARDUINO_EVENT_WIFI_STA_DISCONNECTED (Reason: %u)", reason);
                // Take the "binarySemphrWiFiConnected" to notify to other tasks that WiFi is OFF
                xSemaphoreTakeFromISR(binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
                AT_LOG_V("binarySemphrWiFiConnected set to 0");
                // Suspend all tasks included in "wifiDependentTasks" vector
                suspendWiFiDependentTasks();
                if (!reason)
                    AT_LOG_E("WIFI_STA_DISCONNECTED with reason 0");
                // On reson ASSOC_FAIL wait some time to try to reconnect, otherwise reconnect immediately
                if (reason != WIFI_REASON_ASSOC_FAIL)
                    xSemaphoreGiveFromISR(binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
                break;
            }
            // Got WIFI_STA_GOT_IP event (WiFi connection stablished)
            case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            {
                AT_LOG_I("ARDUINO_EVENT_WIFI_STA_GOT_IP. WiFi connected");
                // Give the "binarySemphrWiFiConnected" to notify to other tasks that WiFi is ON
                xSemaphoreGiveFromISR(binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
                AT_LOG_V("binarySemphrWiFiConnected set to 1");
                // Resume all tasks included in "wifiDependentTasks" vector
                resumeWiFiDependentTasks();
                // Take the "binarySemphrTryToConnectWiFi" to stop reconnecting
                xSemaphoreTakeFromISR(binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
                break;
            }
            default:
                break;
            }

            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        static void WiFiDaemonTask(void *const parameters)
        {
            AT_LOG_I("WiFiDaemonTask created");
            // Create a binary semaphore to know when WiFi is connected
            binarySemphrWiFiConnected = xSemaphoreCreateBinary();
            if (!binarySemphrWiFiConnected)
                AT_LOG_E("Could not create binary semaphore");

            // Create binary semaphore to know when to reconnect WiFi
            binarySemphrTryToConnectWiFi = xSemaphoreCreateBinary();
            if (!binarySemphrTryToConnectWiFi)
                AT_LOG_E("Could not create binary semaphore");
            xSemaphoreGive(binarySemphrTryToConnectWiFi);

            // Create the WiFi connection timeout timer
            static TimerHandle_t timerReconnectWiFi{nullptr};
            timerReconnectWiFi = xTimerCreate(
                "timerReconnectWiFi",
                pdMS_TO_TICKS(WIFI_RECONNECT_WAIT_TIME_MS),
                pdTRUE,
                (void *)0,
                timerReconnectWiFiCB);
            if (!timerReconnectWiFi)
                AT_LOG_E("Could not create timer");
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
                if (xSemaphoreTake(binarySemphrTryToConnectWiFi, portMAX_DELAY))
                {
                    // Connect to WiFi
                    AT_LOG_D("Connecting to %s", wifiSSID);
                    WiFi.begin(wifiSSID, wifiPASS);
                }
            }
        }

        /**
         * Public functions
         */
        void start(const char *const ssid,
                   const char *const passphrase,
                   const UBaseType_t uxPriority)
        {
            if (taskHandle)
            {
                AT_LOG_W("WiFi daemon already started");
                ASSERT(false);
                return;
            }

            wifiSSID = ssid;
            wifiPASS = passphrase;

            xTaskCreatePinnedToCore(
                WiFiDaemonTask,
                "WiFiDaemonTask",
                3 * 1024,
                nullptr,
                uxPriority,
                &taskHandle,
                ARDUINO_RUNNING_CORE);
        }

        void stop()
        {
            if (taskHandle)
            {
                vTaskDelete(taskHandle);
                AT_LOG_I("WiFiDaemon Deleted");
            }
            else
            {
                AT_LOG_W("WiFiDaemon not started");
            }
        }

        BaseType_t blockUntilConnected(const TickType_t xTicksToWait)
        {
            if (binarySemphrWiFiConnected)
                return xQueuePeek(binarySemphrWiFiConnected, (void *)nullptr, xTicksToWait);
            return pdFALSE;
        }

        bool isConnected()
        {
            if (binarySemphrWiFiConnected)
                return uxSemaphoreGetCount(binarySemphrWiFiConnected);
            return false;
        }

        void addDependentTask(TaskHandle_t task)
        {
            // If task is nullptr get the current task handle
            if (!task)
                task = xTaskGetCurrentTaskHandle();
            // Add the task to the "wifiDependentTasks" vector
            wifiDependentTasks.push_back(task);
            // Suspend the task if the WiFi is not enabled
            if (!isConnected())
            {
                AT_LOG_V("Task %s suspended", pcTaskGetName(task));
                vTaskSuspend(task);
            }
        }

    } // namespace WiFiDaemon

} // namespace AT
