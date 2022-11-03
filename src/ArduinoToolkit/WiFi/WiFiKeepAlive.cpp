#include <vector>

#ifdef CORE_DEBUG_LEVEL
#undef CORE_DEBUG_LEVEL
#endif
#define CORE_DEBUG_LEVEL ARDUHAL_LOG_LEVEL_VERBOSE
#include <esp32-hal-log.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Ticker.h>

#define WIFI_RECONNECT_WAIT_TIME_MS 5 * 1000

static const char *wifiSSID;
static const char *wifiPASS;

static SemaphoreHandle_t binarySemphrWiFiConnected = NULL;
static SemaphoreHandle_t binarySemphrTryToConnectWiFi = NULL;

// Create a list with the handles of the task to suspend when WiFi disconnects
static std::vector<TaskHandle_t> wifiDependentTasks;

static void timerReconnectWiFiCB()
{
    if (!uxSemaphoreGetCount(binarySemphrWiFiConnected))
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
        // Did this action unblock a higher priority task?
        if (xHigherPriorityTaskWoken)
            portYIELD_FROM_ISR();
    }
}

static void WiFiEventCB(WiFiEvent_t event, WiFiEventInfo_t info)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    switch (event)
    {
    // Got WIFI_STA_DISCONNECTED event (WiFi disconnected)
    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
    {
        const uint8_t reason = info.wifi_sta_disconnected.reason;
        isr_log_w("ARDUINO_EVENT_WIFI_STA_DISCONNECTED (Reason: %u)", reason);
        // Take the "binarySemphrWiFiConnected" to notify to other tasks that WiFi is OFF
        xSemaphoreTakeFromISR(binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
        isr_log_v("binarySemphrWiFiConnected set to 0");
        // Suspend all tasks included in "wifiDependentTasks" vector
        for (const TaskHandle_t task : wifiDependentTasks)
        {
            vTaskSuspend(task);
            isr_log_v("Task %s suspended", pcTaskGetName(task));
        }
        isr_log_d("WiFi dependent tasks suspended");
        if (!reason)
            log_e("WIFI_STA_DISCONNECTED with reason 0");
        // On reson ASSOC_FAIL wait some time to try to reconnect, otherwise reconnect immediately
        if (reason != WIFI_REASON_ASSOC_FAIL)
            xSemaphoreGiveFromISR(binarySemphrTryToConnectWiFi, &xHigherPriorityTaskWoken);
        break;
    }
    // Got WIFI_STA_GOT_IP event (WiFi connection stablished)
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
    {
        isr_log_i("ARDUINO_EVENT_WIFI_STA_GOT_IP. WiFi connected");
        // Give the "binarySemphrWiFiConnected" to notify to other tasks that WiFi is ON
        xSemaphoreGiveFromISR(binarySemphrWiFiConnected, &xHigherPriorityTaskWoken);
        isr_log_v("binarySemphrWiFiConnected set to 1");
        // Resume all tasks included in "wifiDependentTasks" vector
        for (const TaskHandle_t task : wifiDependentTasks)
        {
            xTaskResumeFromISR(task);
            isr_log_v("Task %s resumed", pcTaskGetName(task));
        }
        isr_log_d("WiFi dependent tasks resumed");
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

static void WiFiKeepAliveTask(void *parameters)
{
    // Create a binary semaphore to know when WiFi is connected
    binarySemphrWiFiConnected = xSemaphoreCreateBinary();
    if (!binarySemphrWiFiConnected)
        log_e("Could not create binary semaphore");

    // Create binary semaphore to know when to reconnect WiFi
    binarySemphrTryToConnectWiFi = xSemaphoreCreateBinary();
    if (!binarySemphrTryToConnectWiFi)
        log_e("Could not create binary semaphore");
    xSemaphoreGive(binarySemphrTryToConnectWiFi);

    // Create the WiFi connection timeout timer
    static Ticker timerReconnectWiFi;
    timerReconnectWiFi.attach_ms(WIFI_RECONNECT_WAIT_TIME_MS, timerReconnectWiFiCB);

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
            log_w("Task %s memory left: %u", pcTaskGetName(NULL), uxTaskGetStackHighWaterMark(NULL));
            // Connect to WiFi
            log_d("Connecting to %s", wifiSSID);
            WiFi.begin(wifiSSID, wifiPASS);
        }
    }
}

TaskHandle_t createWiFiKeepAliveTask(const char *const ssid, const char *const passphrase)
{
    wifiSSID = ssid;
    wifiPASS = passphrase;
    
    static TaskHandle_t taskHandle = NULL;

    xTaskCreatePinnedToCore(
        WiFiKeepAliveTask,
        "WiFiKeepAliveTask",
        3 * 1024,
        nullptr,
        2,
        &taskHandle,
        ARDUINO_RUNNING_CORE);

    return taskHandle;
}

BaseType_t blockUntilWiFiConnected(const TickType_t xTicksToWait)
{
    return xQueuePeek(binarySemphrWiFiConnected, (void *)nullptr, xTicksToWait);
}

bool isWiFiConnected()
{
    return uxSemaphoreGetCount(binarySemphrWiFiConnected);
}

void addWiFiDependentTask(const TaskHandle_t taskHandle)
{
    wifiDependentTasks.push_back(taskHandle);
}
