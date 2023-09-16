#include <LPC17xx.h>
#include "stdio.h"
#include "uart.h"
#include "StarterCode.h"
#include "utility.h"
#include "engine_controller.h"

// struct frame_sequence_item {
//   unsigned int type; //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
//   unsigned int index_in_type; //item's function's index in its respective array. this is a don't care attribute for slack items
//   unsigned int duration; //estimated duration for the slack
//   void (*task)(void); //pointer to task's function
// };
struct task_item {
  unsigned int type; //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
  unsigned int WCET; //WCET in ms
  unsigned int ddl; //relative deadline in ms
  unsigned int min_arr; //minumum interarrival time for sporadic tasks
  void (*task)(void); //pointer to task's function
};
struct true_task_item {
  unsigned int type; //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
  unsigned int WCET; //WCET in ms
  unsigned int absddl; //absolute deadline in us
  unsigned int context_location;
  unsigned int recovered;
  void (*task)(void); //pointer to task's function
};
unsigned int current_frame_index = 0;
unsigned int current_item_index = 0;
unsigned int current_item_running = 0;
//array storing the sequence of tasks to finish in the current frame
struct true_task_item *current_frame_sequence[100];
struct true_task_item *periodic_queue[100];
struct true_task_item *sporadic_queue[100];
struct true_task_item *aperiodic_queue[100];
unsigned int item_number_in_current_sequence = 0;
unsigned int context_in_use[8];
unsigned int context_temp = 0;
unsigned int global_frame_count = 0;
uint32_t i_run_success = 0;
task_function_t p_test_function;
unsigned int first_time = 1;

//global version of framework inputs
unsigned int g_numt;
struct task_item **g_all_tasks=NULL;//pointer to head of array
unsigned int g_nums;
struct task_item **g_sporadic_tasks=NULL;//pointer to head of array
unsigned int g_numa;
struct task_item **g_aperiodic_tasks=NULL;//pointer to head of array
unsigned int g_num_of_frames;
int **g_frame_sequence=NULL;//pointer to head of 2D array
unsigned int *g_item_count_in_each_frame=NULL;//pointer to head of array

void initialize_current_seq (){
	for (unsigned int i = 0; i < 100; i++){
		if ((current_frame_sequence[i]!=NULL)&&(first_time==0)){
			free(current_frame_sequence[i]);
		}
		current_frame_sequence[i] = NULL;
	}
	first_time = 0;
	return;
}
unsigned int find_free_context(){
	for (unsigned int i = 0; i < 7; i++){
		if (context_in_use[i+1] == 0){
			return i+1;
		}
	}
	return 1;//use context 1 if all context are in use
}
void add_item_to_waiting_queue(unsigned int type, //0 for slack, 1 for periodic, 2 for sporadic, 3 for aperiodic
  unsigned int WCET, //WCET in ms
  unsigned int absddl, //absolute deadline in us
  unsigned int context_location, //always 0 for non-periodic
  unsigned int recovered, //always 0 for non-periodic
  void (*task)(void));
void airflow_callback(void)
{
	add_item_to_waiting_queue(2,g_all_tasks[3]->WCET,
				(g_all_tasks[3]->ddl)*1000+timer_read(),
				0, 0, g_all_tasks[3]->task);
}

void vibration_callback(void)
{
    add_item_to_waiting_queue(3,g_all_tasks[4]->WCET,
				(g_all_tasks[4]->ddl)*1000+timer_read(),
				0, 0, g_all_tasks[4]->task);
}
void MyInterruptHandler();
void CyclicExecutionFramework (//arraies of function pointers containing the 3 types of tasks with their size
								unsigned int numt, struct task_item *all_tasks[numt],
								// unsigned int nums, struct task_item *sporadic_tasks[nums],
								// unsigned int numa, struct task_item *aperiodic_tasks[numa],
								//number of frames, array of pointers each pointing to an array containing the task sequence in each frame
								//and the size of each sequence
								unsigned int num_of_frames, int *frame_sequence[num_of_frames],
								unsigned int item_count_in_each_frame[num_of_frames],
								//length of frame in ms
								unsigned int frame_len){
	//globalizing variables
	g_numt=numt;
	g_all_tasks=all_tasks;
	// g_nums=nums;
	// g_sporadic_tasks=sporadic_tasks;
	// g_numa=numa;
	// g_aperiodic_tasks=aperiodic_tasks;
	g_num_of_frames=num_of_frames;
	g_frame_sequence=frame_sequence;
	g_item_count_in_each_frame=item_count_in_each_frame;
	//initializing stuff
	initialize_leds();
	engine_define_task_release_callbacks(airflow_callback, vibration_callback);
	global_frame_count = num_of_frames;
	current_frame_index = 0;
	timer_interrupt_t p_interrupt_hander = &MyInterruptHandler;
	initialize_timer((uint32_t)(frame_len*1000), p_interrupt_hander); //setup the frame boundary iterrupts
	initialize_context_switching_utility();
	//create all 8 contexts
	for (unsigned int i = 0; i < 7; i++){
		uint32_t i_context_created = create_new_context();
		printf("create_new_context() result = %d \n\n", i_context_created);
		context_in_use[i+1]=0;
	}
	context_in_use[0]=1;//no task should run in context 0
	initialize_current_seq();
	current_frame_index = 0;
	NVIC_DisableIRQ(TIMER0_IRQn);
	NVIC_EnableIRQ(TIMER0_IRQn);
	//initialization ends here


	while (1){
		//set line 28 in engine.c to 100, comment back line 142 and 152 and it will work Also set line 28 to 80 work even without commenting back
		//possible bug in starter code: comment back these printfs and line 153 and set line 28 in engine.c to 100
		//printf("running at %d\n", get_currently_active_context());
		if (current_frame_sequence[current_item_index]==NULL){
			//do nothing if no more items in sequence
			//printf("index %d\n", current_item_index);
			continue;
		}
		p_test_function = (current_frame_sequence[current_item_index])->task;
		current_item_running = current_item_index;
		current_item_index++;
		if (((current_frame_sequence[current_item_running])->context_location)!=0){
			//recovered - pause periodic task
			context_switch_to((uint32_t)((current_frame_sequence[current_item_running])->context_location));
			context_in_use[(current_frame_sequence[current_item_running])->context_location]=0;
			//printf("recovering %d\n", (uint32_t)((current_frame_sequence[current_item_running])->context_location));
			//context_switch_to(0);
			//*((uint32_t*)(new_context_stack_pointer + 13 * 4)) = (uint32_t)return_from_context_run;
		}else{
			context_temp = find_free_context();
			//mark context to run in as in use
			context_in_use[context_temp]=1;
			//fetch next item in current sequence or stay idle if nothing to do
			i_run_success = run_in_context((uint32_t)context_temp, p_test_function);
			//unmark context if returned 1
			if (i_run_success==1){
				context_in_use[context_temp]=0;
			}
		}
		//will automatically return to context 0 so no worry
	}
}

void schedule_the_upcoming_frame(){
	unsigned int counter_in_frame_seq = 0;
	unsigned int cumulative_WCET = 0;
	for (unsigned int i = 0; i < g_item_count_in_each_frame[current_frame_index]; i++){
		if (g_frame_sequence[current_frame_index][i]>=0){
			//non-slack item. push directly to queue
			struct true_task_item *temp_item = NULL;
			temp_item = calloc(1, sizeof(struct true_task_item));
			temp_item->type = g_all_tasks[g_frame_sequence[current_frame_index][i]]->type;
			temp_item->WCET = g_all_tasks[g_frame_sequence[current_frame_index][i]]->WCET;
			temp_item->absddl = (g_all_tasks[g_frame_sequence[current_frame_index][i]]->ddl)*1000+timer_read();
			temp_item->context_location = 0;
			temp_item->recovered = 0;
			temp_item->task = g_all_tasks[g_frame_sequence[current_frame_index][i]]->task;
			current_frame_sequence[counter_in_frame_seq]=temp_item;
			counter_in_frame_seq++;
			cumulative_WCET += temp_item->WCET;
		}else{//item value is negative. this means a slack time with duration in ms!!!
			//slack time!
			int slack_duration = 0 - (g_frame_sequence[current_frame_index][i]);
			cumulative_WCET += slack_duration;
			unsigned int task_found = 0;
			struct true_task_item *tempp = NULL;
			//first search in preempteed periodic tasks
			for (unsigned int i=0; i < 100; i++){
				if(task_found == 1){
					break;
				}
				tempp = periodic_queue[i];
				if (tempp == NULL){
					continue;
				}
				if (((tempp->absddl)>(cumulative_WCET*1000+timer_read()))&&((tempp->WCET)<slack_duration)){
					//suitable task found! put it into the queue
					task_found = 1;
					struct true_task_item *temp_item = NULL;
					temp_item = calloc(1, sizeof(struct true_task_item));
					temp_item->type = tempp->type;
					temp_item->WCET = tempp->WCET;
					temp_item->absddl = tempp->absddl;
					temp_item->context_location = tempp->context_location;
					temp_item->recovered = tempp->recovered;
					temp_item->task = tempp->task;
					current_frame_sequence[counter_in_frame_seq]=temp_item;
					counter_in_frame_seq++;
					free(tempp);
					periodic_queue[i]=NULL;
				}
			}
			//now search in sporadic tasks
			for (unsigned int i=0; i < 100; i++){
				if(task_found == 1){
					break;
				}
				tempp = sporadic_queue[i];
				if (tempp == NULL){
					continue;
				}
				if (((tempp->absddl)>(cumulative_WCET*1000+timer_read()))&&((tempp->WCET)<slack_duration)){
					//suitable task found! put it into the queue
					task_found = 1;
					struct true_task_item *temp_item = NULL;
					temp_item = calloc(1, sizeof(struct true_task_item));
					temp_item->type = tempp->type;
					temp_item->WCET = tempp->WCET;
					temp_item->absddl = tempp->absddl;
					temp_item->context_location = tempp->context_location;
					temp_item->recovered = tempp->recovered;
					temp_item->task = tempp->task;
					current_frame_sequence[counter_in_frame_seq]=temp_item;
					counter_in_frame_seq++;
					free(tempp);
					sporadic_queue[i]=NULL;
				}
			}
			//now search in aperiodic tasks
			for (unsigned int i=0; i < 100; i++){
				if(task_found == 1){
					break;
				}
				tempp = aperiodic_queue[i];
				if (tempp == NULL){
					continue;
				}
				if (((tempp->absddl)>(cumulative_WCET*1000+timer_read()))&&((tempp->WCET)<slack_duration)){
					//suitable task found! put it into the queue
					task_found = 1;
					struct true_task_item *temp_item = NULL;
					temp_item = calloc(1, sizeof(struct true_task_item));
					temp_item->type = tempp->type;
					temp_item->WCET = tempp->WCET;
					temp_item->absddl = tempp->absddl;
					temp_item->context_location = tempp->context_location;
					temp_item->recovered = tempp->recovered;
					temp_item->task = tempp->task;
					current_frame_sequence[counter_in_frame_seq]=temp_item;
					counter_in_frame_seq++;
					free(tempp);
					aperiodic_queue[i]=NULL;
				}
			}
		}
	}
}

void add_item_to_waiting_queue(unsigned int type, //0 for slack, 1 for periodic, 2 for sporadic, 3 for aperiodic
  unsigned int WCET, //WCET in ms
  unsigned int absddl, //absolute deadline in us
  unsigned int context_location, //always 0 for non-periodic
  unsigned int recovered, //always 0 for non-periodic
  void (*task)(void)){
	if(type == 0){
		printf("Error: adding slack item to waiting queue!\n");
		return;
	}
	struct true_task_item *temp_item = NULL;
	temp_item = calloc(1, sizeof(struct true_task_item));
	temp_item->type = type;
	temp_item->WCET = WCET;
	temp_item->absddl = absddl;
	temp_item->context_location = context_location;
	temp_item->recovered = recovered;
	temp_item->task = task;
	if (type == 1){
		//add to recover queue
		for (unsigned int i = 0; i < 100; i++){
			if (periodic_queue[i] == NULL){//empty slot in queue
				periodic_queue[i] = temp_item;
				break;
			}else if((uint32_t)(periodic_queue[i]->absddl) < timer_read()){//expired item in queue
				free(periodic_queue[i]);
				periodic_queue[i] = temp_item;
				break;
			}
			if (i==99){
				printf("Error: peo queue full!\n");
			}
		}
	}else if (type == 2){
		//add to sporadic queue
		for (unsigned int i = 0; i < 100; i++){
			if (sporadic_queue[i] == NULL){//empty slot in queue
				sporadic_queue[i] = temp_item;
				break;
			}else if((uint32_t)(sporadic_queue[i]->absddl) < timer_read()){//expired item in queue
				free(sporadic_queue[i]);
				sporadic_queue[i] = temp_item;
				break;
			}
			if (i==99){
				printf("Error: spo queue full!\n");
			}
		}
	}else if (type == 3){
		//add to aperiodic queue
		for (unsigned int i = 0; i < 100; i++){
			if (aperiodic_queue[i] == NULL){//empty slot in queue
				aperiodic_queue[i] = temp_item;
				break;
			}else if((uint32_t)(aperiodic_queue[i]->absddl) < timer_read()){//expired item in queue
				free(aperiodic_queue[i]);
				aperiodic_queue[i] = temp_item;
				break;
			}
			if (i==99){
				printf("Error: ape queue full!\n");
			}
		}
	}else{
		printf("Error: unexpected type to add!\n");
		return;
	}
}


//below is the function called at each frame boundary
void MyInterruptHandler()
{
	uint32_t i_current_context = get_currently_active_context();
	// printf ("Interrupt fired during context %d\n", i_current_context);
	// printf ("Timer read: %d\n", timer_read()); 
	if (i_current_context != 0) { 
		if (((current_frame_sequence[current_item_running])->type)==1){//periodic running
			if (((current_frame_sequence[current_item_running])->recovered)==0){
				//non-recovered periodic. add to the queue
				//printf("RECOVERED ONE TASK\n");
				add_item_to_waiting_queue(1,(current_frame_sequence[current_item_running])->WCET,
				(current_frame_sequence[current_item_running])->absddl,
				i_current_context, 1, (current_frame_sequence[current_item_running])->task);
				//printf("adding task %d with absddl %d to wait queue!\n", (current_frame_sequence[current_item_running])->WCET,
				//(current_frame_sequence[current_item_running])->absddl);
			}else{
				//recovered peiodic. flush away
				context_in_use[i_current_context]=0;
			}
		}else{
			//not periodic. also flush away
			context_in_use[i_current_context]=0;
		}
		context_switch_to(0);
	}
	current_item_running++;
	while((current_item_running < 100) && (current_frame_sequence[current_item_running]!=NULL)){
		if (((current_frame_sequence[current_item_running])->type)==1){
			//recover other items in the sequence
			add_item_to_waiting_queue(1,(current_frame_sequence[current_item_running])->WCET,
				(current_frame_sequence[current_item_running])->absddl,
				0, 1, (current_frame_sequence[current_item_running])->task);
		}
		current_item_running++;
	}
	initialize_current_seq();
	current_frame_index = (current_frame_index + 1) % global_frame_count;
	current_item_index = 0;
	schedule_the_upcoming_frame();
}

int main(){
	struct task_item t0;
	struct task_item t1;
	struct task_item t2;
	struct task_item t3;
	struct task_item t4;
	t0=(struct task_item){
	.type = 1, //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
	.WCET = 20, //WCET in ms
	.ddl = 600,//relative deadline in ms
	.min_arr = 0, //minumum interarrival time for sporadic tasks
	.task=&engine_low_pressure_compression_shaft_task, //pointer to task's function
	};
	t1=(struct task_item){
	.type = 1, //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
	.WCET = 20, //WCET in ms
	.ddl = 300,//relative deadline in ms
	.min_arr = 0, //minumum interarrival time for sporadic tasks
	.task=&engine_high_pressure_compression_shaft_task, //pointer to task's function
	};
	t2=(struct task_item){
	.type = 1, //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
	.WCET = 10, //WCET in ms
	.ddl = 150,//relative deadline in ms
	.min_arr = 0, //minumum interarrival time for sporadic tasks
	.task=&engine_fuel_injection_task, //pointer to task's function
	};
	t3=(struct task_item){
	.type = 2, //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
	.WCET = 50, //WCET in ms
	.ddl = 200,//relative deadline in ms
	.min_arr = 400, //minumum interarrival time for sporadic tasks
	.task=&engine_airflow_monitoring_task, //pointer to task's function
	};
	t4=(struct task_item){
	.type = 3, //0 for slack, 1 for periodic,2 for sporadic, 3 for aperiodic
	.WCET = 30, //WCET in ms
	.ddl = 500,//relative deadline in ms
	.min_arr = 0, //minumum interarrival time for sporadic tasks
	.task=&engine_vibration_analysis_task, //pointer to task's function
	};
	
	struct task_item *test_a[5];
	test_a[0]=&t0;
	test_a[1]=&t1;
	test_a[2]=&t2;
	test_a[3]=&t3;
	test_a[4]=&t4;

	int test_b_0[3] = {0,2,-70};
	int test_b_1[3] = {1,2,-70};
	int test_b_2[2] = {2,-90};
	int test_b_3[2] = {2,-90};
	int test_b_4[3] = {1,2,-70};
	int test_b_5[2] = {2,-90};
	int *test_b[6] = {test_b_0,test_b_1,test_b_2,test_b_3,test_b_4,test_b_5};
	unsigned int test_c[6] = {3,3,2,2,3,2};

	
	CyclicExecutionFramework(5, test_a, 6, test_b, test_c, 100);//!!!!!!!


	while(1) /* you don't need to keep this */
	{
		; /* loop indefinitely */
	}
}
// Schedule for Part 2
// LOW PRES ARRIVALS:0 				WCET20
// HIGH PRES ARRIVALS:100, 400 			WCET20
// FUEL INJEC ARRIVALS:0, 100, 200, 300, 400, 500 	WCET10

// 0-99MS:LOW PRES, FUEL INJEC, 70MS SLACK
// 100-199MS:HISH PRES, FUEL INJEC, 70MS SLACK
// 200-299MS:FUEL INJEC, 90MS SLACK
// 300-399MS:FUEL INJEC, 90MS SLACK
// 400-499MS:HISH PRES, FUEL INJEC, 70MS SLACK
// 500-599MS:FUEL INJEC, 90MS SLACK
