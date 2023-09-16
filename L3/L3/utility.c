#include <LPC17xx.h>
#include <stdlib.h>

#include "utility.h"

static timer_interrupt_t _global_timer_callback = NULL;

void initialize_timer(uint32_t timer_interval_us, timer_interrupt_t callback)
{
    SystemCoreClockUpdate();
    
    LPC_SC->PCONP   |= 2 | 4;
    
	LPC_TIM0->PR    = (SystemCoreClock / 4 / 1000000) - 1; /* count up by us */
	LPC_TIM0->MR0   = timer_interval_us; /* how many us to count */
	LPC_TIM0->MCR   = 2 | 1;
    
    LPC_TIM1->PR    = (SystemCoreClock / 4 / 1000000) - 1; /* count up by us */
	LPC_TIM1->MCR   = 0; /* just let it naturally wrap around at 2^32 */
    
    _global_timer_callback = callback;
	NVIC_EnableIRQ(TIMER0_IRQn);
    
    
	LPC_TIM0->TCR   = 1;
	LPC_TIM1->TCR   = 1;
}

uint32_t timer_read(void)
{
    return LPC_TIM1->TC;
}

void TIMER0_IRQHandler(void)
{
    LPC_TIM0->IR |= 1;
    _global_timer_callback();
}


#define SHPR3 *(uint32_t*)0xE000ED20
#define _ICSR *(uint32_t*)0xE000ED04

#define CONTEXT_STACK_SIZE 0x200

uint32_t* _global_resume_stack_pointer = NULL;
uint32_t* _global_save_stack_pointer = NULL;

static uint32_t _global_stack_pointers[MAX_CONTEXT];

/* this starts at 1, the initial stack is context 0 */
static uint32_t _global_next_available_context = 1;

/* the current context running */
static uint32_t _global_currently_active_context = 0;

/* the context numbers that run_in_context was called
   from for a given context */
static uint32_t _global_run_in_context_origins[MAX_CONTEXT];

/* did a context run to completion? */
static uint32_t _global_context_ran_to_completion[MAX_CONTEXT];

static void return_from_context_run(void)
{
    _global_context_ran_to_completion[_global_currently_active_context] = 1;
    context_switch_to(_global_run_in_context_origins[_global_currently_active_context]);
}

void initialize_context_switching_utility(void)
{
    SHPR3 |= 0xFF << 16;
}

uint32_t create_new_context(void)
{
    if(_global_next_available_context >= MAX_CONTEXT)
    {
        /* fail if there's no more contexts available */
        return 0xFFFFFFFF;
    }
    
    _global_next_available_context += 1;
	return _global_next_available_context - 1;
}

uint32_t run_in_context(uint32_t context, task_function_t fn)
{
    /* the first stack begins at the address in MSR */
    /* yes this is an intentional NULL dereference, no I do not know why
       the ARM designers put it at address 0, but here we are... */
    /* the compiler will complain (as it should honestly) without the volatile */
    uint32_t MSR_register_original_value = *((volatile uint32_t*)0);
    
    /* leave space for the initial stack pointer */
    uint32_t new_context_stack_pointer = MSR_register_original_value - context * CONTEXT_STACK_SIZE;
    
    /* reserve spots for all 16 registers that would be saved by the start of a context switch */
    new_context_stack_pointer -= 16 * 4;
    
    /* the top saved register is the PSR
       bit T (24) of the PSR must always be set or the system will hard fault */
    *((uint32_t*)(new_context_stack_pointer + 15 * 4)) = (1 << 24);
    
    /* the second-to-top saved register is the PC
       this is the code we want to run, so set it to the supplied function */
    *((uint32_t*)(new_context_stack_pointer + 14 * 4)) = (uint32_t)fn;
    
    /* the third-to-top saved register is the LR
       this is where the supplied function should return to,
       in this case we're going to pick a function that just context switches back */
    *((uint32_t*)(new_context_stack_pointer + 13 * 4)) = (uint32_t)return_from_context_run;
    
    _global_stack_pointers[context] = new_context_stack_pointer;
    _global_run_in_context_origins[context] = _global_currently_active_context;
    _global_context_ran_to_completion[context] = 0;
    
    context_switch_to(context);
    
    return _global_context_ran_to_completion[context];
}

void context_switch_to(uint32_t context)
{
    /* tell the PendSV exception where we're going */
    _global_resume_stack_pointer = &(_global_stack_pointers[context]);
    
    /* and where we're coming from */
    _global_save_stack_pointer = &(_global_stack_pointers[_global_currently_active_context]);
    
    /* now switch the active context */  
    _global_currently_active_context = context;
    
    /* trigger PendSV */
    _ICSR |= 1<<28;
    
    /* force the PendSV to happen, no reordering past here */
	__asm("isb");
    
    /* if we got here the function has run to completion, so we return normally
        likely to run_in_context */
}

uint32_t get_currently_active_context(void)
{
    return _global_currently_active_context;
}


static const unsigned char led_pos[8] = { 28, 29, 31, 2, 3, 4, 5, 6 };

void set_led(int n) 
{
	int mask = 1 << led_pos[n];
	if (n < 3)
    {
        LPC_GPIO1->FIOSET = mask;
    }
    else 
    {
        LPC_GPIO2->FIOSET = mask;
    }
}

void clear_led(int n) 
{
	int mask = 1 << led_pos[n];
	if (n < 3) 
    {
        LPC_GPIO1->FIOCLR = mask;
    }
	else 
    {
        LPC_GPIO2->FIOCLR = mask;
    }
}

void initialize_leds(void)
{
    // initialize the LEDs
	LPC_GPIO1->FIODIR |= 0xB0000000;
	LPC_GPIO2->FIODIR |= 0x0000007C; 
}

