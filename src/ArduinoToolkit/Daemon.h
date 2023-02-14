#pragma once

#include <Arduino.h>

namespace AT
{

    class Daemon
    {
    protected:
        ~Daemon();
        inline TaskHandle_t &getTaskHandle() { return m_taskHandle; }
        static void setDaemonCreatedFlag(bool &daemonCreatedFlag);

    private:
        TaskHandle_t m_taskHandle{nullptr};
    }; // class Daemon

} // namespace AT
