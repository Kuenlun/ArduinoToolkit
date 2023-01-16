#include <NTPClient.h>
#include <WiFiUdp.h>

#include "WiFiKeepAlive.h"

#include "NTPClientTask.h"

namespace AT
{

    namespace NTPClientTask
    {

        static bool taskCreatedFlag{false};
        static uint32_t updateTimeClientPeriodMs;

        static void GetDateTimeTask(void *const parameters)
        {
            // Define NTP Client to get time
            static WiFiUDP ntpUDP;
            static NTPClient timeClient(ntpUDP);

            // Initialize a NTPClient to get time
            timeClient.begin();
            // Set offset time in seconds to adjust for your timezone, for example:
            // GMT +1 = 3600
            timeClient.setTimeOffset(3600);

            AT::WiFiKeepAlive::addDependentTask(xTaskGetCurrentTaskHandle());

            log_d("Update time client");
            while (true)
            {
                if (timeClient.forceUpdate())
                {
                    log_v("Current time is: %s", timeClient.getFormattedTime());
                    vTaskDelay(pdMS_TO_TICKS(updateTimeClientPeriodMs));
                }
                else
                {
                    log_w("Could not update timeClient");
                    continue;
                }
            }
        }

        TaskHandle_t createTask(const uint32_t &updatePeriodMs)
        {
            // Trying to create the task again?
            if (taskCreatedFlag)
            {
                log_e("WiFi Keep Alive task has already been created");
                ESP_ERROR_CHECK(ESP_FAIL);
            }

            static TaskHandle_t taskHandle = nullptr;

            updateTimeClientPeriodMs = updatePeriodMs;

            xTaskCreatePinnedToCore(
                GetDateTimeTask,
                "GetDateTimeTask",
                3 * 1024,
                nullptr,
                1,
                &taskHandle,
                ARDUINO_RUNNING_CORE);

            // Set the taskCreatedFlag to true
            taskCreatedFlag = true;

            return taskHandle;
        }

    } // namespace NTPClientTask

} // namespace AT
