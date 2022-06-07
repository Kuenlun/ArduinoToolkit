#pragma once

#include "Logger.h"


class TimedExecutionBase
{
public:
    virtual void timedExecution(const unsigned long waitTime) const = 0;
};


class TimedExecution : public TimedExecutionBase
{
public:
    TimedExecution(TimedExecutionBase& component)
        : m_component(component)
    {
    }

    void timedExecution(const unsigned long waitTime) const override
    {
        static unsigned long lastExecutionTime = waitTime;
        if (millis() - lastExecutionTime >= waitTime) {
            lastExecutionTime = millis();
            m_component.timedExecution(waitTime);
        }
    }
private:
    TimedExecutionBase& m_component;
};
