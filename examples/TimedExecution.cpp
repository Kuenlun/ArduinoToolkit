#include "Tools/TimeConversions.h"
#include "Tools/Logger.h"
#include "Tools/TimedExecution.h"


void foo()
{
    LOG_INFO("Working...");
}

void bar(const unsigned long a)
{
    LOG_INFO("Working with parameters: %d", a);
}


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    Serial.begin(115200);
    Tools::logger.config("TIMED EXEC");
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
    static unsigned long i = 0;
    // "foo" will be called every seond
    TIMED_EXECUTION(Tools::secsToMillis(1), foo);
    // "bar" will be called every 2 seconds
    TIMED_EXECUTION(Tools::secsToMillis(2), bar, i++);
}
