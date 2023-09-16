/* This file contains utility functions for:
    - setting up a timer interrupt
    - reading the current timer count
    - context switching
    - setting LEDs
    
    Use these as part of your scheduler for Lab 3*/
    
#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <stdint.h>

#ifndef EXT_FN

#ifdef __cplusplus
#define EXT_FN extern "C"
#else
#define EXT_FN
#endif

#endif

/* the timer interrupt is a void functions returning void */
typedef void (*timer_interrupt_t)();

/* initialize and start the timer interrupt, triggers at an interval defined in microseconds */
EXT_FN void initialize_timer(uint32_t timer_interval_us, timer_interrupt_t callback);

/* read the timer counter value in microseconds, 
    REMEMBER: this wraps around at 2^32 */
EXT_FN uint32_t timer_read(void);


/* you can create at most 7 (numbered 1-8) contexts, 
    the 0th one is the original context on startup */
#define MAX_CONTEXT 8

/* tasks are void functions returning void */
typedef void (*task_function_t)();

/* configures the processor to enable context switching */
EXT_FN void initialize_context_switching_utility(void);

/* create a new execution context (mostly just a stack)
   - this can be reused across multiple functions
   - do not create more than MAX_CONTEXTs contexts
   - if a job in a task needs to be cancelled or aborted, 
   just leave the context alone, the next time you call run_in_context 
   on that context, it will be reset as a part of running the function */
EXT_FN uint32_t create_new_context(void);

/* runs a function in an execution context
   - this resets the context if it perviously had any saved state 
   - this switches exection to that context
   - this will return back into the context it was called in
   - if the function ran to completion, this returns 1 
   - if we returned from this because we context switched back here, this returns 0 */
EXT_FN uint32_t run_in_context(uint32_t context, task_function_t fn);

/* switches context to the given context, resuming execution from the saved state
   - if called from an interrupt, the context switch will occur after the interrupt has returned */
EXT_FN void context_switch_to(uint32_t context);

/* what context is the currently active code running in?
   - will return zero for the default context before any context switching is done
   - if called from an interrupt, will give the context that was running when the interrupt fired
   HINT: use this to see if there's a tardy task in your frame time interrupt */
EXT_FN uint32_t get_currently_active_context(void);

/* configures the LEDs */
EXT_FN void initialize_leds(void);

/* turns on an LED 0-7 */
EXT_FN void set_led(int n);

/* turns off an LED 0-7 */
EXT_FN void clear_led(int n);

#endif

