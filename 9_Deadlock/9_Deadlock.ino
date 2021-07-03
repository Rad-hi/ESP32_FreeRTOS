/**
 * ESP32 Dining Philosophers
 * 
 * The classic "Dining Philosophers" problem in FreeRTOS form.
 * 
 * Based on http://www.cs.virginia.edu/luther/COA2/S2019/pa05-dp.html
 * 
 * Date: February 8, 2021
 * Author: Shawn Hymel
 * License: 0BSD
 */

// You'll likely need this on vanilla FreeRTOS
//#include <semphr.h>

// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Printing params
#define QUEUE_LEN         5         // max number of items the queue could hold
#define BUFF_LEN          255       // Buffer length i.e. how many chars to print

// Settings
#define NUM_TASKS         5         // Number of tasks (philosophers)
#define TASK_STACK_SIZE   2048      // Bytes in ESP32, words in vanilla FreeRTOS

static QueueHandle_t msg_q;         // Serial queue

// Globals
static SemaphoreHandle_t bin_sem;   // Wait for parameters to be read
static SemaphoreHandle_t done_sem;  // Notifies main task when done
static SemaphoreHandle_t chopstick[NUM_TASKS];


// No Serial call anywhere out of here 
void print_messages(void *params){
  // Hold the item from the queue
  char buff[BUFF_LEN];

  // Super-loop
  while(1){
    // Check if there's a message waiting in the queue (non-blocking)
    //             //The queue //item addrs //timeout in ticks
    if(xQueueReceive(msg_q, (void*)&buff, 0) == pdTRUE){ // pdTRUE if something was read from the queue
                                                         // (pdFALSE else)
      Serial.println(buff);
    }
  }
}

// Task: eating
void eat(void *parameters) {

  int num;
  char buf[50];

  int f, s;

  // Copy parameter and increment semaphore count
  num = *(int *)parameters;
  xSemaphoreGive(bin_sem);

  switch(num){
    case 0:
    case 1:
    case 2:
    case 3:
      f = num;
      s = (num + 4) % NUM_TASKS;
    break;
    case 4:
      f = 0;
      s = 4;
    break;
    default: break;
  }
  
  // Take first chopstick
  xSemaphoreTake(chopstick[f], portMAX_DELAY);

  sprintf(buf, "Philosopher %i took chopstick %i", num, f);
  // Made this to protect the serial port from multiple threads accessing it at a time
  xQueueSend(msg_q, (void*)&buf, 0);
  
  // Add some delay to force deadlock
  vTaskDelay(1 / portTICK_PERIOD_MS);

  // Take second chopstick
  xSemaphoreTake(chopstick[s], portMAX_DELAY);

  sprintf(buf, "Philosopher %i took chopstick %i", num, s);
  xQueueSend(msg_q, (void*)&buf, 0);
  
  // Do some eating
  sprintf(buf, "Philosopher %i is eating", num);
  xQueueSend(msg_q, (void*)&buf, 0);
  
  vTaskDelay(10 / portTICK_PERIOD_MS);

  // Put down second chopstick
  xSemaphoreGive(chopstick[s]);

  sprintf(buf, "Philosopher %i returned chopstick %i", num, s);
  xQueueSend(msg_q, (void*)&buf, 0);
  
  // Put down first chopstick
  xSemaphoreGive(chopstick[f]);
  
  sprintf(buf, "Philosopher %i returned chopstick %i", num, f);
  xQueueSend(msg_q, (void*)&buf, 0);
  
  // Notify main task and delete self
  xSemaphoreGive(done_sem);
  vTaskDelete(NULL);
}

void setup() {

  char task_name[20];

  Serial.begin(115200);
  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Create the queue
  msg_q = xQueueCreate(QUEUE_LEN, BUFF_LEN);
  
  // Create kernel objects before starting tasks
  bin_sem = xSemaphoreCreateBinary();
  done_sem = xSemaphoreCreateCounting(NUM_TASKS, 0);
  for (int i = 0; i < NUM_TASKS; i++) {
    chopstick[i] = xSemaphoreCreateMutex();
  }

  // 1st Task --> Serial printer
  xTaskCreatePinnedToCore(print_messages, // Function to run
                          "Print msgs",    // Name of task
                          1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                          NULL,           // Params to pass to function
                          1,              // Task priority (0 to configMAX_PRIORITIES -1)
                          NULL,           // Task handle
                          app_cpu);       // Run on one core

  // Have the philosphers start eating
  for (int i = 0; i < NUM_TASKS; i++) {
    sprintf(task_name, "Philosopher %i", i);
    xTaskCreatePinnedToCore(eat,
                            task_name,
                            TASK_STACK_SIZE,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }


  // Wait until all the philosophers are done
  for (int i = 0; i < NUM_TASKS; i++) {
    xSemaphoreTake(done_sem, portMAX_DELAY);
  }

  char fin [] = "Done! No deadlock occurred!";
  xQueueSend(msg_q, (void*)&fin, 0);
}

void loop() {
  // Do nothing in this task
}
