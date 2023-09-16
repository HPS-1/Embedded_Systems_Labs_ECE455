#include <LPC17xx.h>

#include "engine_controller.h"
#include "utility.h"

static void engine_busy_wait(uint32_t ms)
{
    uint32_t start = timer_read();
    while(timer_read() < start + ms * 1000)
    {
        ; /* busy wait */
    }
}

static engine_task_release_callback_t release_airflow_callback;
static engine_task_release_callback_t release_vibration_callback;

void engine_define_task_release_callbacks(engine_task_release_callback_t release_airflow, engine_task_release_callback_t release_vibration)
{
    release_airflow_callback = release_airflow;
    release_vibration_callback = release_vibration;
}

void engine_low_pressure_compression_shaft_task(void)
{
    static uint32_t counter = 0;
    set_led(0);
    engine_busy_wait(10);
    
    if (counter == 0)
    {
        release_airflow_callback();
    }
    counter = (counter + 1) % 3;
    
    engine_busy_wait(8);
    clear_led(0);
}

void engine_high_pressure_compression_shaft_task(void)
{
    static uint32_t counter = 0;
    set_led(1);
    counter = (counter + 1) % 17;
    
    if (counter == 13)
    {
        set_led(7);
        engine_busy_wait(85);
        clear_led(7);
    }
    else
    {
        engine_busy_wait(17);
    }
    
    clear_led(1);
}

void engine_fuel_injection_task(void)
{
    static uint32_t counter = 0;
    set_led(2);
    engine_busy_wait(3);
        
    if (counter == 5)
    {
        release_vibration_callback();
    }
    counter = (counter + 1) % 15;
    
    engine_busy_wait(4);
    clear_led(2);
}

void engine_airflow_monitoring_task(void)
{
    set_led(3);
    engine_busy_wait(45);
    clear_led(3);
}

void engine_vibration_analysis_task(void)
{
    set_led(4);
    engine_busy_wait(26);
    clear_led(4);
}

