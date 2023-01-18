/**
 * Based on Arvind Ravulavaru sketch <https://github.com/arvindr21>
 */

#pragma once

#include <Arduino.h>

namespace AT
{

    namespace OTA
    {

        void executeOTA(const char *const url, const uint16_t port = 80);

    } // namespace OTA

} // namespace AT
