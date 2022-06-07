#include "Tools/Logger.h"
#include "Tools/TimedExecution.h"



class WorkToDo : public TimedExecutionBase
{
    void timedExecution(const unsigned long waitTime) const
    {
        LOG_INFO("Working...");
    }
};


WorkToDo workToDo;
TimedExecution timedExecution(workToDo);


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    Tools::logger.config("TimedExecution");
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    timedExecution.timedExecution(Tools::secsToMillis(1));
}
