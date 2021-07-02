// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// max number of items the queue could hold
#define QUEUE_LEN       5

// Buffer length i.e. how many chars to print
#define BUFF_LEN        255 // CLI buffer

// AVG buffer length
#define AVG_LEN         10

// Pins
#define LED_PIN         25
#define ADC_PIN         A0

// Timer params
#define TIMER_DIV       8         // Clock = 80MHz <--> Timer = 10MHz
#define TIMER_MAX_C     1000000   

// Globals
static QueueHandle_t msg_q;                 // Serial queue
static hw_timer_t *timer = NULL;
static volatile uint8_t which = 0;          // Which buffer to write to
static volatile uint8_t w_idx = 0;          // Writing index
static volatile int buff_l[AVG_LEN];        // Lower half of the double buffer
static volatile int buff_h[AVG_LEN];        // Higher half of the double buffer
static volatile float average = 0;          // The calculated average
static SemaphoreHandle_t bin_sem = NULL;    // Signals to the task to calculate & print

// ISR
void IRAM_ATTR on_timer_isr(){

  BaseType_t task_woken = pdFALSE;

  int val = analogRead(ADC_PIN);

  if(!which){       // Write to low buffer 
                    // (which == 0 --> write to Low, read from High)
    buff_l[w_idx++] = val;
  }
  else{             // Write to high buffer 
    buff_h[w_idx++] = val;
  }

  if(w_idx > 9){    // We filled a buffer --> Can calculate AVG
    w_idx = 0;
    which = !which; // Flip the buffers
    
    // Signal to calculating task to calculate avg
    xSemaphoreGiveFromISR(bin_sem, &task_woken);

    if(task_woken) portYIELD_FROM_ISR();
  }
}

// AVG calculator
void avg(void* params){
  while(1){
    // Can calculate
    xSemaphoreTake(bin_sem, portMAX_DELAY);
  
    if(which){ // Read from Low buffer
      for(int i = 0; i < AVG_LEN; i++){
        average += buff_l[i];
      }
    }
    else{     // Read from High buffer
      for(int i = 0; i < AVG_LEN; i++){
        average += buff_h[i];
      }
    }
    average /= AVG_LEN; 
  }
}

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

// CLI --> Reads and echoes Serial commands
//         If recieve "avg", calculate the average of the buffer(the one to be read from) and print it through msg_q
void CLI(void *params){

  char s[BUFF_LEN];
  char c;
  uint8_t idx = 0;
  
  while(1){
    
    // Read the serial content, echo it through the msg_q and test for delay values
    if(Serial.available() > 0){ // If data is available to be read
      c = Serial.read(); // Read byte

      // Values must be terminated with a newline char
      if(c != '\n'){
        s[idx++] = c;
      }
      else{
        // Turn read chars into a string
        s[idx] = '\0';
        
        // Send string to printing task through msg_q
        xQueueSend(msg_q, (void*)&s, 0);

        // Find avg command
        uint8_t s_idx = String(s).indexOf("avg");
        if(s_idx == 0){ // There's an avg command at bigennig of line
          String avg_ = String(average);
          xQueueSend(msg_q, (void*)&avg_, 0);
        }
        
        // Clear buffer and reset index
        memset(s, 0, BUFF_LEN); // Clear the buffer
        idx = 0; // Reset the index
      }
    }
  }
}

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  msg_q    = xQueueCreate(QUEUE_LEN, BUFF_LEN);

  // Create the semaphore
  bin_sem = xSemaphoreCreateBinary();

  if(bin_sem == NULL){
    Serial.println("Couldn't create semaphore");
    ESP.restart();
  }

  // 1st Task to run forever --> Serial printer
  xTaskCreatePinnedToCore(print_messages, // Function to run
                          "Print msgs",    // Name of task
                          1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                          NULL,           // Params to pass to function
                          1,              // Task priority (0 to configMAX_PRIORITIES -1)
                          NULL,           // Task handle
                          app_cpu);       // Run on one core

  // CLI task creation
  xTaskCreatePinnedToCore(CLI,
                          "CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // AVG calculator task creation
  xTaskCreatePinnedToCore(avg,
                          "AVG",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);

  // Create and start the timer
  //               //num° //divider //count-up
  timer  = timerBegin(0, TIMER_DIV, true);

  // Provide the ISR to the timer 
  //                 //timer //ISR         //edge
  timerAttachInterrupt(timer, &on_timer_isr, true);

  // At what value the ISR shall trigger
  //            //timer //count to  //auto-reload
  timerAlarmWrite(timer, TIMER_MAX_C, true);

  // Enable the ISR to trigger
  timerAlarmEnable(timer);

  vTaskDelete(NULL);
}

void loop() {
  // Do nothing
}
