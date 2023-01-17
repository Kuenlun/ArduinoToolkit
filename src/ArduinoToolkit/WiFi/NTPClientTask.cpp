#include <NTPClient.h>
#include <WiFiUdp.h>

#include "WiFiKeepAlive.h"

#include "NTPClientTask.h"

namespace AT
{

    namespace NTPClientTask
    {

        // Define NTP Client to get time
        static WiFiUDP ntpUDP;
        static NTPClient timeClient(ntpUDP);

        static uint32_t s_updateDateTimePeriodTicks;
        static uint32_t s_retryUpdateDateTimePeriodTicks;

        static void timerUpdateDateTimeCB(const TimerHandle_t xTimer)
        {
            BaseType_t xHigherPriorityTaskWoken{pdFALSE};
            if (WiFiKeepAlive::isConnected())
            {
                if (timeClient.forceUpdate())
                {
                    isr_log_d("Current time is: %s", timeClient.getFormattedTime());
                    if (xTimerGetPeriod(xTimer) != s_updateDateTimePeriodTicks)
                    {
                        xTimerChangePeriodFromISR(xTimer, s_updateDateTimePeriodTicks, &xHigherPriorityTaskWoken);
                        isr_log_v("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(s_updateDateTimePeriodTicks));
                    }
                }
                else
                {
                    isr_log_w("Could not update timeClient");
                    if (xTimerGetPeriod(xTimer) != s_retryUpdateDateTimePeriodTicks)
                    {
                        xTimerChangePeriodFromISR(xTimer, s_retryUpdateDateTimePeriodTicks, &xHigherPriorityTaskWoken);
                        isr_log_v("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(s_retryUpdateDateTimePeriodTicks));
                    }
                }
            }
            else
            {
                isr_log_d("Could not update timeClient because WiFi is not connected");
                if (xTimerGetPeriod(xTimer) != s_retryUpdateDateTimePeriodTicks)
                {
                    xTimerChangePeriodFromISR(xTimer, s_retryUpdateDateTimePeriodTicks, &xHigherPriorityTaskWoken);
                    isr_log_v("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(s_retryUpdateDateTimePeriodTicks));
                }
            }
            // Did this action unblock a higher priority task?
            if (xHigherPriorityTaskWoken)
                portYIELD_FROM_ISR();
        }

        void createDaemon(const TickType_t &updateDateTimePeriodTicks,
                          const TickType_t &retryUpdateDateTimePeriodTicks)
        {
            static bool taskCreatedFlag{false};
            // Trying to create the task again?
            if (taskCreatedFlag)
            {
                log_e("WiFi Keep Alive task has already been created");
                ESP_ERROR_CHECK(ESP_FAIL);
            }

            // Initialize a NTPClient to get time
            timeClient.begin();
            // Set offset time in seconds to adjust for your timezone, for example:
            // GMT +1 = 3600
            timeClient.setTimeOffset(3600);

            s_updateDateTimePeriodTicks = updateDateTimePeriodTicks;
            s_retryUpdateDateTimePeriodTicks = retryUpdateDateTimePeriodTicks;

            // Create the update datetime timer
            static TimerHandle_t timerUpdateDateTime{nullptr};
            timerUpdateDateTime = xTimerCreate(
                "timerUpdateDatetime",
                retryUpdateDateTimePeriodTicks,
                pdTRUE,
                (void *)0,
                timerUpdateDateTimeCB);
            if (!timerUpdateDateTime)
                log_e("Could not create timer");
            // Start the timer
            xTimerStart(timerUpdateDateTime, portMAX_DELAY);

            // Set the taskCreatedFlag to true
            taskCreatedFlag = true;
        }

    } // namespace NTPClientTask

} // namespace AT
