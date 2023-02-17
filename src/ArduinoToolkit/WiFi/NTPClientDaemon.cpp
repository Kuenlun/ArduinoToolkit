#include <WiFiUdp.h>

#include "WiFiDaemon.h"

#include "NTPClientDaemon.h"

namespace AT
{

    // WiFiUdp object
    static WiFiUDP ntpUDP;

    /**
     * NTPClientDaemon class static variables
     */
    NTPClient NTPClientDaemon::s_timeClient(ntpUDP);
    uint32_t NTPClientDaemon::s_updateDateTimePeriodTicks;
    uint32_t NTPClientDaemon::s_retryUpdateDateTimePeriodTicks;

    /**
     * NTPClientDaemon class protected methods
     */
    NTPClientDaemon::NTPClientDaemon(const TickType_t updateDateTimePeriodTicks,
                                     const TickType_t retryUpdateDateTimePeriodTicks)
        : m_timerUpdateDateTime(nullptr)
    {
        // Initialize static variables
        s_updateDateTimePeriodTicks = updateDateTimePeriodTicks;
        s_retryUpdateDateTimePeriodTicks = retryUpdateDateTimePeriodTicks;

        log_v("Instanciating NTPClientDaemon object");

        // Initialize a NTPClient to get time
        s_timeClient.begin();
        // Set offset time in seconds to adjust for your timezone, for example:
        // GMT +1 = 3600
        s_timeClient.setTimeOffset(3600);

        // Create the update datetime timer
        m_timerUpdateDateTime = xTimerCreate(
            "timerUpdateDatetime",
            retryUpdateDateTimePeriodTicks,
            pdTRUE,
            (void *)0,
            timerUpdateDateTimeCB);
        if (!m_timerUpdateDateTime)
            log_e("Could not create timer");
        // Start the timer
        xTimerStart(m_timerUpdateDateTime, portMAX_DELAY);
    }

    NTPClientDaemon::~NTPClientDaemon()
    {
        xTimerStop(m_timerUpdateDateTime, portMAX_DELAY);
        s_timeClient.end();
        log_i("NTPClientDaemon Deleted");
    }

    /**
     * NTPClientDaemon class private methods
     */
    void NTPClientDaemon::changeTimerPeriodToNormal(const TimerHandle_t &xTimer,
                                                    BaseType_t *const pxHigherPriorityTaskWoken)
    {
        if (xTimerGetPeriod(xTimer) != s_updateDateTimePeriodTicks)
        {
            xTimerChangePeriodFromISR(xTimer, s_updateDateTimePeriodTicks, pxHigherPriorityTaskWoken);
            isr_log_v("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(s_updateDateTimePeriodTicks));
        }
    }

    void NTPClientDaemon::changeTimerPeriodToRetry(const TimerHandle_t &xTimer,
                                                   BaseType_t *const pxHigherPriorityTaskWoken)
    {
        if (xTimerGetPeriod(xTimer) != s_retryUpdateDateTimePeriodTicks)
        {
            xTimerChangePeriodFromISR(xTimer, s_retryUpdateDateTimePeriodTicks, pxHigherPriorityTaskWoken);
            isr_log_v("Update DateTime timer period changed to %ums", pdTICKS_TO_MS(s_retryUpdateDateTimePeriodTicks));
        }
    }

    void NTPClientDaemon::timerUpdateDateTimeCB(const TimerHandle_t xTimer)
    {
        BaseType_t xHigherPriorityTaskWoken{pdFALSE};
        if (WiFiDaemon::isConnected())
        {
            if (s_timeClient.forceUpdate())
            {
                isr_log_d("Current time is: %s", s_timeClient.getFormattedTime());
                changeTimerPeriodToNormal(xTimer, &xHigherPriorityTaskWoken);
            }
            else
            {
                isr_log_w("Could not update timeClient");
                changeTimerPeriodToRetry(xTimer, &xHigherPriorityTaskWoken);
            }
        }
        else
        {
            isr_log_w("Could not update timeClient because WiFi is not connected");
            changeTimerPeriodToRetry(xTimer, &xHigherPriorityTaskWoken);
        }
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }

} // namespace AT
