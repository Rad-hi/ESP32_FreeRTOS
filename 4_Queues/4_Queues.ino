// Use only one core just for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// max number of items the queue could hold
#define QUEUE_LEN       5

// Buffer length i.e. how many chars to print
#define BUFF_LEN        255 // CLI buffer
#define EXCHANGE_LEN    8   // Task comm buffer

// Pins
#define LED_PIN         25

// Global queue handle --> accessible to all tasks
static QueueHandle_t msg_q;
static QueueHandle_t first_q;
static QueueHandle_t second_q;


// Task 1: preferrably once created, it'd handle all printing tasks 
// --> no Serial call anywhere out of here (well ideally)
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

// Task A --> Prints any messages in second_q
//            Reads and echoes Serial commands
//            If recieve "delay xxxx", send "xxxx" to task B through first_q
void a_task(void *params){

  char s[BUFF_LEN];
  char exchange[EXCHANGE_LEN];
  char c;
  uint8_t idx = 0;
  
  while(1){
    
    // If there's a message to be printed, send it to the printing task through msg_q
    if(xQueueReceive(second_q, (void*)&exchange, 0) == pdTRUE){
      xQueueSend(msg_q, (void*)&exchange, 5);
    }

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
        xQueueSend(msg_q, (void*)&s, 5);

        // Find delay command
        uint8_t s_idx = String(s).indexOf("delay ");
        if(s_idx == 0){ // There's a delay command at bigennig of line
          char d[4];
          for(int i = s_idx + 6; i < s_idx + 10; i++){
            d[i-6] = isDigit(s[i]) ? s[i] : '\0'; // Parse the delay period
          }
          // Turn the delay period to an integer
          int t = strtol(d, NULL, 10);
          
          // Put the delay period in first_q for the other task to parse it
          xQueueSend(first_q, (void*)&t, 5);
        }
        
        // Clear buffer and reset index
        memset(s, 0, BUFF_LEN); // Clear the buffer
        idx = 0; // Reset the index
      }
    }
  }
}

// Task B --> Reads delay time 't' from first_q
//            Blinks LED with 't'
//            If LED blinks 100 times, send "Blinked" to A through second_q
void b_task(void *params){
  int t = 200; // default blinking period
  uint8_t count = 0;
  static const String blinked = "Blinked";
  
  while(1){
    
    // Update the blinking count if there's any
    xQueueReceive(first_q, (void*)&t, 0);
    
    // Blink the LED
    digitalWrite(LED_PIN, 1);
    vTaskDelay(t / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, 0);
    vTaskDelay(t / portTICK_PERIOD_MS);

    count++;
  
    if(count == 100 && xQueueSend(second_q, (void*)&blinked, 10) == pdTRUE) count = 0;
  }
}

void setup() {
  Serial.begin(115200);
  // Give time for the serial connection to be established
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  pinMode(LED_PIN, OUTPUT);

  // Assign the created queue to the handle
  //                   //Queue len //Size of each queue item (any number of bytes)
  msg_q    = xQueueCreate(QUEUE_LEN, BUFF_LEN);
  first_q  = xQueueCreate(QUEUE_LEN, EXCHANGE_LEN);
  second_q = xQueueCreate(QUEUE_LEN, EXCHANGE_LEN);

  // 1st Task to run forever --> Serial printer
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                print_messages, // Function to run
                "Print msgs",    // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,           // Task handle
                app_cpu);       // Run on one core
                
  // 2nd Task to run forever --> Task A
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                a_task,         // Function to run
                "A",            // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,           // Task handle
                app_cpu);       // Run on one core

                
  // 3rd Task to run forever --> Task B
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                b_task,         // Function to run
                "B",            // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,           // Task handle
                app_cpu);       // Run on one core

  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Nothing
}
