// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Settings
#define NUM_PRODUCERS               5 // Num of producer tasks
#define NUM_CONSUMERS               2 // Num of consumer tasks
#define NUM_WRITES                  3 // Num times each producer writes to buff
#define BUFF_SIZE                   5 // Num of available slots

// Globals
static int buf[BUFF_SIZE];            // Shared buffer (circular)
static int head = 0;                  // Writing index to buffer
static int tail = 0;                  // Reading index to buffer
static SemaphoreHandle_t mutex;       // To protect Serial port as a shared ressource
static SemaphoreHandle_t bin_sem;     // Waits for parameter to be read
static SemaphoreHandle_t fill_sem;    // number of filled slots
static SemaphoreHandle_t empty_sem;   // number of empty slots

// Producer: write a given number of times to shared buffer
void producer(void *parameters) {

  int num = *(int *)parameters;

  // Release the binary semaphore
  xSemaphoreGive(bin_sem);

  // Fill shared buffer with task number
  for (int i = 0; i < NUM_WRITES; i++) {

    // wait for an empty slot
    xSemaphoreTake(empty_sem, portMAX_DELAY);
    
    // Take mutex
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    // Critical section (accessing shared buffer)
    buf[head] = num;
    head = (head + 1) % BUFF_SIZE;

    // Give the mutex (make it available)
    xSemaphoreGive(mutex);    

    // Signal a filled slot to consumer
    xSemaphoreGive(fill_sem);
    
  }

  // Delete self task
  vTaskDelete(NULL);
}

// Consumer: continuously read from shared buffer
void consumer(void *parameters) {

  int val;

  // Read from buffer
  while (1) {

    // Wait for a value
    xSemaphoreTake(fill_sem, portMAX_DELAY);

    // Take mutex
    xSemaphoreTake(mutex, portMAX_DELAY);
    
    // Critical section (accessing shared buffer and Serial)
    val = buf[tail];
    tail = (tail + 1) % BUFF_SIZE;    
    Serial.println(val);
    
    // Give the mutex (make it available)
    xSemaphoreGive(mutex);
    
    // Indicate that an slot has been freed up
    xSemaphoreGive(empty_sem);
  }
}

void setup() {

  char task_name[12];
  
  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  Serial.println("Semaphores task STARTED!");

  // Create mutexes and semaphores before starting tasks
  mutex     = xSemaphoreCreateMutex();
  bin_sem   = xSemaphoreCreateBinary();
  //                                //max count //initial 
  fill_sem  = xSemaphoreCreateCounting(BUFF_SIZE, 0);
  empty_sem = xSemaphoreCreateCounting(BUFF_SIZE, BUFF_SIZE);
  
  // Start producer tasks (wait for each to read argument)
  for (int i = 0; i < NUM_PRODUCERS; i++) {
    sprintf(task_name, "Producer %i", i);
    xTaskCreatePinnedToCore(producer,
                            task_name,
                            1024,
                            (void *)&i,
                            1,
                            NULL,
                            app_cpu);
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  }

  // Start consumer tasks
  for (int i = 0; i < NUM_CONSUMERS; i++) {
    sprintf(task_name, "Consumer %i", i);
    xTaskCreatePinnedToCore(consumer,
                            task_name,
                            1024,
                            NULL,
                            1,
                            NULL,
                            app_cpu);
  }

  // Take mutex (protect Serial port
  xSemaphoreTake(mutex, portMAX_DELAY);

  // Notify that all tasks have been created
  Serial.println("All tasks created");

  // Give the mutex (make it available)
  xSemaphoreGive(mutex);
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
