/**
 * Based on Arvind Ravulavaru sketch <https://github.com/arvindr21>
 */

#include <Update.h>

#include "ArduinoToolkit/WiFi/OTA_AWS_S3.h"

namespace AT
{

    namespace OTA
    {

        static WiFiClient s_wifiClient;

        struct HostBin
        {
        public:
            HostBin(const std::string_view &url)
            {
                const auto splitCharIdx{url.find_last_of("/")};
                m_host = url.substr(0, splitCharIdx);
                m_bin = url.substr(splitCharIdx + 1);
            }

            inline const std::string &getHost() const { return m_host; }
            inline const std::string &getBin() const { return m_bin; }

        private:
            std::string m_host;
            std::string m_bin;
        };

        struct headerResponseSteps_t
        {
            bool statusCode : 1;
            bool contentType : 1;
            bool contentLength : 1;
            bool endOfHeader : 1;
        };

        static size_t readServerResponseHeader()
        {
            size_t outContentLength{0}; // Where return value will be stored
            headerResponseSteps_t headerResponseSteps{
                .statusCode = false,
                .contentType = false,
                .contentLength = false,
                .endOfHeader = false};
            while (s_wifiClient.available())
            {
                const String line{s_wifiClient.readStringUntil('\n')};
                AT_LOG_V("%s", line.c_str());

                // Check if the HTTP response is 200
                if (!headerResponseSteps.statusCode)
                {
                    if (static constexpr char stringBeginning[]{"HTTP/1.1"};
                        line.startsWith(stringBeginning))
                    {
                        const std::string_view responseValue{line.c_str() + sizeof(stringBeginning)};
                        if (line.indexOf("200 OK") < 0)
                        {
                            AT_LOG_E("Got a non 200 status code from server: %s", responseValue.data());
                            return 0;
                        }
                        headerResponseSteps.statusCode = true;
                        continue;
                    }
                }

                // Extract the content type
                if (!headerResponseSteps.contentType)
                {
                    if (static constexpr char stringBeginning[]{"Content-Type:"};
                        line.startsWith(stringBeginning))
                    {
                        const std::string_view responseValue{line.c_str() + sizeof(stringBeginning)};
                        AT_LOG_D("Got %s payload", responseValue.data());
                        if (responseValue.find("application/octet-stream") != std::string::npos)
                        {
                            AT_LOG_I("Response Content-Type is valid");
                        }
                        else
                        {
                            AT_LOG_E("Response Content-Type is not valid");
                            return 0;
                        }
                        headerResponseSteps.contentType = true;
                        continue;
                    }
                }

                // Extract content length
                if (!headerResponseSteps.contentLength)
                {
                    if (static constexpr char stringBeginning[]{"Content-Length:"};
                        line.startsWith(stringBeginning))
                    {
                        const std::string_view responseValue{line.c_str() + sizeof(stringBeginning)};
                        AT_LOG_D("Content length is %s", responseValue.data());
                        outContentLength = std::strtoul(responseValue.data(), nullptr, 10);
                        headerResponseSteps.contentLength = true;
                        continue;
                    }
                }

                // End of header
                if (!headerResponseSteps.endOfHeader)
                {
                    if (line.length() == 1)
                    {
                        headerResponseSteps.endOfHeader = true;
                        break;
                    }
                }
            }

            if (headerResponseSteps.statusCode &&
                headerResponseSteps.contentType &&
                headerResponseSteps.contentLength &&
                headerResponseSteps.endOfHeader)
            {
                return outContentLength;
            }
            else
            {
                AT_LOG_E("Invalid server header response");
                return 0;
            }
        }

        static size_t requestS3BinFile(const std::string &host, const std::string &bin, const uint16_t port)
        {
            if (s_wifiClient.connect(host.c_str(), port))
            {
                AT_LOG_I("Connection to host succeeded");
                // Send HTTP request header
                s_wifiClient.printf("GET /%s HTTP/1.0\n", bin.c_str());
                s_wifiClient.printf("Host: %s\n", host.c_str());
                s_wifiClient.printf("Connection: close\n\n");
                AT_LOG_D("HTTP request header sent");

                // Wait for server response
                const TickType_t timeStartTicks{xTaskGetTickCount()};
                static constexpr TickType_t requestTimeoutTicks{pdMS_TO_TICKS(5000)};
                while (!s_wifiClient.available())
                {
                    if (xTaskGetTickCount() - timeStartTicks > requestTimeoutTicks)
                    {
                        AT_LOG_E("Client timeout");
                        s_wifiClient.stop();
                        return 0;
                    }
                    vTaskDelay(pdMS_TO_TICKS(100));
                }

                // Read the server response header
                return readServerResponseHeader();
            }
            else
            {
                // Connection failed
                AT_LOG_E("Could not connect to host: %s on port %u", host.c_str(), port);
            }
            return 0;
        }

        /**
         * "executeOTA(OTA_URL)" does not work when the current
         * partition scheme is "huge_app.csv"
         */
        void executeOTA(const char *const url, const uint16_t port)
        {
            // Split the URL into host and bin
            const HostBin hostBinStruct(url);
            AT_LOG_V("Host: %s", hostBinStruct.getHost().c_str());
            AT_LOG_V("Bin:  %s", hostBinStruct.getBin().c_str());

            // Connect to AWS S3 and request the bin file
            const size_t contentLength{requestS3BinFile(hostBinStruct.getHost(), hostBinStruct.getBin(), port)};
            if (!contentLength)
            {
                s_wifiClient.stop();
                return;
            }

            // Fetch the bin file and update the ESP32
            if (Update.begin(contentLength))
            {
                AT_LOG_I("OTA update started");
                const size_t written = Update.writeStream(s_wifiClient);
                // Check how many bytes have been written
                if (written == contentLength)
                    AT_LOG_D("Written %lu / %lu", written, contentLength);
                else
                    AT_LOG_W("Written %lu / %lu", written, contentLength);
                // Check if update completed successfully
                if (Update.end())
                {
                    AT_LOG_I("Update successfully completed");
                    AT_LOG_I("ESP can now be rebooted");
                }
                else
                {
                    AT_LOG_E("An error occurred during the update: %u", Update.getError());
                }
            }
            else
            {
                AT_LOG_E("Not enough space to begin OTA");
            }
            s_wifiClient.flush();
            s_wifiClient.stop();
        }

    } // namespace OTA

} // namespace AT
