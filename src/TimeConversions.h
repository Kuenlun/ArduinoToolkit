#pragma once

#include <Arduino.h>


namespace Tools
{

    // Constexpr seconds to milliseconds (no overflow check)
    template <typename T>
    constexpr T secsToMillis(const T seconds) { return 1000 * seconds; }
    // Constexpr minutes to milliseconds (no overflow check)
    template <typename T>
    constexpr T minsToMillis(const T minutes) { return 60 * 1000 * minutes; }
    // Constexpr hours to milliseconds (no overflow check)
    template <typename T>
    constexpr T hoursToMillis(T hours) { return 60 * 60 * 1000 * hours; }
    // Constexpr days to milliseconds (no overflow check)
    template <typename T>
    constexpr T daysToMillis(T days) { return 24 * 60 * 60 * 1000 * days; }

    // Constexpr milliseconds to seconds
    template <typename T>
    constexpr T millisToSecs(const T millis) { return millis / 1000; }
    // Constexpr milliseconds to minutes
    template <typename T>
    constexpr T millisToMins(const T millis) { return millis / (1000 * 60); }
    // Constexpr milliseconds to hours
    template <typename T>
    constexpr T millisToHours(const T millis) { return millis / (1000 * 60 * 60); }
    // Constexpr milliseconds to days
    template <typename T>
    constexpr T millisToDays(const T millis) { return millis / (1000 * 60 * 60 * 24); }

}
