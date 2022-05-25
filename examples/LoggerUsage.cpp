/**
 * To completely disable the logger define the macro "LOG_DISABLE"
 * Must be defined before the include of the file "Logger.h"
 * Best practice is to define it in the "platform.ini" file as follows:
*  build_flags =
*      -D LOG_DISABLE
 */
//#define LOG_DISABLE
#include "Logger.h"


/* * * * * *
 *  SETUP  *
 * * * * * */
void setup()
{
    // Begin Serial as usual
    Serial.begin(115200);
    // Configure the logger with a name, the logging format and the level
    Tools::logger.config(   "TEST",
                            "[%T:%e] %@ (%l) %n: %v\n",
                            Tools::Logger::Level::trace);

    /**
     * Some logging examples
     */
    LOG_TRACE("This text will be printed in the same way as print");
    LOG_DEBUG("This text will be formatted in the same way as %s", "printf");
    LOG_INFO(String("Also works with Arduino String"));
    LOG_WARN(std::string("Also works with std::string"));
    LOG_ERROR(7);       // Works in the same way as Serial.print()
    LOG_FATAL(32, HEX); // Works in the same way as Serial.print()

    // The logger can be configured anywhere in the code
    Tools::logger.config(   "TEST2",
                            Tools::Logger::DEFAULT_PATTERN,
                            Tools::Logger::Level::warn);
    LOG_TRACE("This text will NOT be printed");
    LOG_DEBUG("This text will NOT be printed");
    LOG_INFO("This text will NOT be printed");
    LOG_WARN("This text will be printed");
    LOG_ERROR("This text will be printed");

    Tools::logger.setLevel(Tools::Logger::Level::off);
    LOG_FATAL("This text will NOT be printed");
}

/* * * * * *
 *  LOOP   *
 * * * * * */
void loop()
{
}
