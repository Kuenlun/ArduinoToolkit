#pragma once

#include <Arduino.h>


namespace Tools {

    /**
     * @brief Struct that contains the wrapper function
     * Remark 1: For simpler implementation use the macro given below
     * Remark 2: I could not find a better way to implement a wrapper function
     * 
     * @tparam t_WAIT_TIME  Time to wait for the wrapped function to execute (in milliseconds)
     * @tparam t_INIT_EXEC  If the function will be executed the first time
     * @tparam F            Function type (as given by "decltype" function)
     * @tparam f            Function to execute (must return void)
     */
    template<unsigned long t_WAIT_TIME, bool t_INIT_EXEC, class F, F f>
    struct TimedExecution;

    template<unsigned long t_WAIT_TIME, bool t_INIT_EXEC, class... Args, void(*f)(Args...)>
    struct TimedExecution<t_WAIT_TIME, t_INIT_EXEC, void(*)(Args...), f>
    {
        static void wrap(Args... args)
        {
            static unsigned long lastExecutionTime = t_INIT_EXEC ? millis() - t_WAIT_TIME : millis();
            if (millis() - lastExecutionTime >= t_WAIT_TIME) {
                lastExecutionTime = millis();
                f(args...);
            }
        }
    };

}


/**
 * @brief Macro to simplify the call of the wrapper function
 * 
 * @tparam WAIT_TIME    Time to wait for the wrapped function to execute (in milliseconds)
 * @tparam INIT_EXEC    If the function will be executed the first time
 * @tparam FUNC         Function to execute (must return void)
 * @param  __VA_ARGS__  Parameters of the function
 */
#define TIMED_EXECUTION(WAIT_TIME, INIT_EXEC, FUNC, ...)\
    Tools::TimedExecution<WAIT_TIME, INIT_EXEC, decltype(&FUNC), FUNC>::wrap(__VA_ARGS__)
