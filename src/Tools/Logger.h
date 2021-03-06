#pragma once

#include <Arduino.h>
#include "TimeConversions.h"


namespace Tools {

    class Logger
    {
    public:
        enum class Level : uint8_t
        {
            trace  = 0,
            debug  = 1,
            info   = 2,
            warn   = 3,
            error  = 4,
            fatal  = 5,
            off    = 6
        };

        static constexpr char DEFAULT_NAME[] = "LOGGER";
        static constexpr char DEFAULT_PATTERN[] = "[%T:%e] (%l) %n: %v\n";
        static constexpr Level DEFAULT_LEVEL = Level::trace;

    public:
        Logger( const char* name=DEFAULT_NAME,
                const char* const pattern=DEFAULT_PATTERN,
                const Level level=DEFAULT_LEVEL)
            : m_name(name),
              m_pattern(pattern),
              m_level(level)
        {
        }

        void config(const char* const name,
                    const char* const pattern=DEFAULT_PATTERN,
                    const Level level=DEFAULT_LEVEL)
        {
            setName(name);
            setLevel(level);
            setPattern(pattern);
        }

        void config(const char* const name,
                    const Level level,
                    const char* const pattern=DEFAULT_PATTERN)
        {
            setName(name);
            setLevel(level);
            setPattern(pattern);
        }

        void setName(const char* const loggerName) { m_name = loggerName; }
        const char* getName() const { return m_name; }

        /**
         * @brief Set the logging pattern (based on spdlog format)
         * Example: const char* pattern = "[%T:%e] (%l) %n: %v\n"
         * Output: [HH:MM:SS:mmm] (WARNING) LOGGER_NAME: TEXT_TO_BE_LOGGED
         * 
         * @param pattern const char* with the format
         * %v -> User text to be logged
         * %n -> Name of the logger
         * %l -> Full log level
         * %L -> Short log level
         * %H -> Hours
         * %M -> Minutes
         * %S -> Seconds
         * %e -> Milliseconds
         * %R -> %H:%M
         * %T -> %H:%M:%S
         * %X -> %H:%M:%S
         * %@ -> %s:%#
         * %s -> Basename of source file
         * %g -> Full or relative path to source file (__FILE__)
         * %# -> Source line
         * %! -> Source function
         */
        void setPattern(const char* const pattern=DEFAULT_PATTERN) {
            m_pattern = pattern;
        }

        const char* getPattern() const { return m_pattern; }

        void setLevel(const Level level) { m_level = level; }
        Level getLevel() const { return m_level; }

        template <typename... Args>
        void log(   const Level level,
                    const char* const file,
                    const int line,
                    const char* const func,
                    const Args... args) const
        {
            // Skip logging if given level is lower than "m_level"
            if (level < m_level)
                return;

            // Get the current time
            const unsigned long currentTime = millis();
            const uint16_t mSeconds = currentTime % 1000;
            const uint8_t seconds = millisToSecs(currentTime) % 60;
            const uint8_t minutes = millisToMins(currentTime) % 60;
            const uint8_t hours = millisToHours(currentTime) % 24;

            // When false -> copy pattern character
            // When true  -> format pattern
            bool formatFlag = false; 
            for (uint16_t i = 0; i < strlen(m_pattern); i++) {
                if (formatFlag) {
                    formatFlag = false;
            
                    // Format based on the pattern
                    switch (m_pattern[i])
                    {
                    case 'v':
                        // User text
                        _printUserText(args...);
                        break;
                    case 'n':
                        // Name of the logger
                        Serial.print(m_name);
                        break;
                    case 'l':
                        // Full log level
                        Serial.printf("%*s", 7, _levelToStr(level));
                        break;
                    case 'L':
                        // Short log level
                        Serial.print(_levelToStr(level)[0]);
                        break;
                    case 'H':
                        // Hours
                        Serial.printf("%02d", hours);
                        break;
                    case 'M':
                        // Minutes
                        Serial.printf("%02d", minutes);
                        break;
                    case 'S':
                        // Seconds
                        Serial.printf("%02d", seconds);
                        break;
                    case 'e':
                        // Milliseconds
                        Serial.printf("%03d", mSeconds);
                        break;
                    case 'R':
                        // %H:%M
                        Serial.printf("%02d:%02d", hours, minutes);
                        break;
                    case 'T':
                        // %H:%M:%S
                        Serial.printf("%02d:%02d:%02d", hours, minutes, seconds);
                        break;
                    case 'X':
                        // %H:%M:%S
                        Serial.printf("%02d:%02d:%02d", hours, minutes, seconds);
                        break;
                    case 'g':
                        // Full or relative path to source file (__FILE__)
                        Serial.print(file);
                        break;
                    case 's':
                        // Basename of source file
                        {
                            const char* const backSlashPos = strrchr(file, '\\');
                            backSlashPos ? Serial.print(backSlashPos + 1) : Serial.print(file);
                        }
                        break;
                    case '#':
                        // Source line
                        Serial.print(line);
                        break;
                    case '@':
                        // %s:%#
                        {
                            const char* const backSlashPos = strrchr(file, '\\');
                            backSlashPos ? Serial.print(backSlashPos + 1) : Serial.print(file);
                            Serial.print(':');
                            Serial.print(line);
                        }
                        break;
                    case '!':
                        // Source function
                        Serial.print(func);
                        break;
                    default:
                        break;
                    }
                } else {
                    if (m_pattern[i] == '%')
                        formatFlag = true;
                    else
                        Serial.print(m_pattern[i]);
                }
            }
        }

        template <typename... Args>
        void logRaw(const Level level,
                    const Args... args) const
        {
            // Skip logging if given level is lower than "m_level"
            if (level < m_level)
                return;
            _printUserText(args...);
        }

        void emptyLine(const Level level) const
        {
            // Skip logging if given level is lower than "m_level"
            if (level < m_level)
                return;
            Serial.println();
        }
    
    private:
        template <typename... Args>
        static void _printUserText(const Args... args)
        {
            Serial.print(args...);
        }

        template <typename... Args>
        static void _printUserText(const char* const format, const Args... args)
        {
            Serial.printf(format, args...);
        }

        static void _printUserText(const std::string& x)
        {
            Serial.print(x.c_str());
        }

        static const char* const _levelToStr(const Level level)
        {
            switch (level)
            {
                case Level::trace: return "TRACE";
                case Level::debug: return "DEBUG";
                case Level::info:  return "INFO";
                case Level::warn:  return "WARNING";
                case Level::error: return "ERROR";
                case Level::fatal: return "FATAL";
                default:           return nullptr;
            }
        }

    private:
        const char* m_name;
        const char* m_pattern;
        Level       m_level;
    };


    // Object for client logger
    extern Logger logger;

}


/**
 * Macros for using logging methods
 * This methods should be used only with the macros due to usability and
 * to remove code from release versions by defining "LOG_DISABLE" macro.
 */
#if defined LOG_DISABLE
    // Normal log functions
    #define LOG_TRACE(...)
    #define LOG_DEBUG(...)
    #define LOG_INFO(...)
    #define LOG_WARN(...)
    #define LOG_ERROR(...)
    #define LOG_FATAL(...)
    // Raw log functions
    #define LOG_TRACE_RAW(...)
    #define LOG_DEBUG_RAW(...)
    #define LOG_INFO_RAW(...)
    #define LOG_WARN_RAW(...)
    #define LOG_ERROR_RAW(...)
    #define LOG_FATAL_RAW(...)
    // New line
    #define LOG_TRACE_NL(NUM)
    #define LOG_DEBUG_NL(NUM)
    #define LOG_INFO_NL(NUM)
    #define LOG_WARN_NL(NUM)
    #define LOG_ERROR_NL(NUM)
    #define LOG_FATAL_NL(NUM)
#else
    // Normal log functions
    #define LOG_TRACE(...)      ::Tools::logger.log(Tools::Logger::Level::trace,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_DEBUG(...)      ::Tools::logger.log(Tools::Logger::Level::debug,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_INFO(...)       ::Tools::logger.log(Tools::Logger::Level::info,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_WARN(...)       ::Tools::logger.log(Tools::Logger::Level::warn,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_ERROR(...)      ::Tools::logger.log(Tools::Logger::Level::error,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    #define LOG_FATAL(...)      ::Tools::logger.log(Tools::Logger::Level::fatal,\
                                __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
    // Raw log functions
    #define LOG_TRACE_RAW(...)  ::Tools::logger.logRaw(Tools::Logger::Level::trace, __VA_ARGS__)
    #define LOG_DEBUG_RAW(...)  ::Tools::logger.logRaw(Tools::Logger::Level::debug, __VA_ARGS__)
    #define LOG_INFO_RAW(...)   ::Tools::logger.logRaw(Tools::Logger::Level::info, __VA_ARGS__)
    #define LOG_WARN_RAW(...)   ::Tools::logger.logRaw(Tools::Logger::Level::warn, __VA_ARGS__)
    #define LOG_ERROR_RAW(...)  ::Tools::logger.logRaw(Tools::Logger::Level::error, __VA_ARGS__)
    #define LOG_FATAL_RAW(...)  ::Tools::logger.logRaw(Tools::Logger::Level::fatal, __VA_ARGS__)
    // New line
    #define LOG_TRACE_NL()      ::Tools::logger.emptyLine(Tools::Logger::Level::trace)
    #define LOG_DEBUG_NL()      ::Tools::logger.emptyLine(Tools::Logger::Level::debug)
    #define LOG_INFO_NL()       ::Tools::logger.emptyLine(Tools::Logger::Level::info)
    #define LOG_WARN_NL()       ::Tools::logger.emptyLine(Tools::Logger::Level::warn)
    #define LOG_ERROR_NL()      ::Tools::logger.emptyLine(Tools::Logger::Level::error)
    #define LOG_FATAL_NL()      ::Tools::logger.emptyLine(Tools::Logger::Level::fatal)
#endif
