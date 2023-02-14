#include "Daemon.h"

namespace AT
{

    Daemon::~Daemon()
    {
        if (m_taskHandle)
        {
            log_i("Deleting %s task", pcTaskGetName(m_taskHandle));
            vTaskDelete(m_taskHandle);
        }
    }

    void Daemon::setDaemonCreatedFlag(bool &daemonCreatedFlag)
    {
        // Trying to create the task again?
        if (daemonCreatedFlag)
        {
            log_e("Daemon has already been created");
            ESP_ERROR_CHECK(ESP_FAIL);
        }
        // Set the flag to true
        daemonCreatedFlag = true;
    }

} // namespace AT
