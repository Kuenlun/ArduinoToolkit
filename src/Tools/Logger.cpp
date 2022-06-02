#include "Logger.h"


namespace Tools {

    // Needed for the linker to work
    constexpr char Logger::DEFAULT_NAME[];
    constexpr char Logger::DEFAULT_PATTERN[];
    constexpr Logger::Level Logger::DEFAULT_LEVEL;

    // Object for client logger
    Logger logger;

}
