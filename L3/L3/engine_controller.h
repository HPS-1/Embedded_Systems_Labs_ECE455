/* This file contains the task functions for Lab 3
    Include this file directly in your C/C++ implementation
    of Lab 3, and call the functions directly each time
    one of the jobs in the engine controller should run.

   NOTE: make sure the timer and leds (utlity.h) are initialized before 
    trying to run any engine tasks */
    
#ifndef _ENGINE_CONTROLLER_H_
#define _ENGINE_CONTROLLER_H_

#ifndef EXT_FN

#ifdef __cplusplus
#define EXT_FN extern "C"
#else
#define EXT_FN
#endif

#endif

/* NOTE: make sure the timer and leds (utlity.h) are initialized before trying to run any engine tasks */

/* the engine controller will dispatch the airflow and vibration tasks using these callbacks,
   that is: you should define functions that release the airflow and vibration tasks to your cyclic executive, and pass
   them in here */

typedef void (*engine_task_release_callback_t)();
EXT_FN void engine_define_task_release_callbacks(engine_task_release_callback_t release_airflow, engine_task_release_callback_t release_vibration);

/* these are the tasks */
EXT_FN void engine_low_pressure_compression_shaft_task(void);
EXT_FN void engine_high_pressure_compression_shaft_task(void);
EXT_FN void engine_fuel_injection_task(void);
EXT_FN void engine_airflow_monitoring_task(void);
EXT_FN void engine_vibration_analysis_task(void);



#endif
