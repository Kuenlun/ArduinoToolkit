#pragma once

#include <Arduino.h>

namespace AT
{

    template <typename CRTP>
    class Daemon
    {
    public:
        template <typename... Args>
        static std::shared_ptr<CRTP> instance(Args &&...args)
        {
            std::shared_ptr<CRTP> sharedInstance;
            if (isInstanced())
            {
                sharedInstance = s_instance.lock();
                log_w("Daemon already created");
            }
            else
            {
                // Thanks to Robert Puskas for this temporary subclass that allows
                // to use std::make_unique and std::make_shared with private constructors
                struct MkShrdEnablr : public CRTP
                {
                    MkShrdEnablr(Args &&...args)
                        : CRTP(std::forward<Args>(args)...){};
                };
                sharedInstance = std::make_shared<MkShrdEnablr>(std::forward<Args>(args)...);
                s_instance = sharedInstance;
            }
            return sharedInstance;
        }

        inline static bool isInstanced() { return s_instance.expired() ? false : true; }

        static void assertIsInstanced()
        {
            if (!isInstanced())
            {
                log_e("Daemon not created");
                ESP_ERROR_CHECK(ESP_ERR_INVALID_STATE);
            }
        }

    protected:
        // Constructor, default implementation
        Daemon() = default;
        // Destructor, default implementation
        ~Daemon() = default;

    private:
        // Copy constructor, deleted to prevent unintentional copies
        Daemon(const Daemon &) = delete;
        // Move constructor, deleted to prevent unintentional moves
        Daemon(Daemon &&) = delete;
        // Copy assignment operator, deleted to prevent unintentional assignments
        Daemon &operator=(const Daemon &) = delete;
        // Move assignment operator, deleted to prevent unintentional assignments
        Daemon &operator=(Daemon &&) = delete;

    private:
        static std::weak_ptr<CRTP> s_instance;
    }; // class Daemon

    template <typename CRTP>
    std::weak_ptr<CRTP> Daemon<CRTP>::s_instance;

} // namespace AT
