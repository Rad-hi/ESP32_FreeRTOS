// Use only one core just for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Buffer length i.e. how many chars to read
#define BUFF_LEN       255


// Flag for serial printer task to print and free memory
/*
 * Volatile is mandatory since can_print is being modified by two different tasks in
 * separate stacks --> compiler can't see that happening, so it optimizes it away.
 */
static volatile uint8_t can_print = 0;

// Serial buffer, initially empty
char *buff = NULL;

// Task handles which allow for inspection
static TaskHandle_t parse_ = NULL;
static TaskHandle_t print_ = NULL;


// Task for printing data to the serial monitor and freeing memory
void serial_printer(void *params){
  // Task super-loop
  while(1){
    if(can_print){
      // Echo the buffer content
      Serial.println(buff);
      // Free the allocated memory
      vPortFree(buff);
      can_print = 0;
      buff = NULL;
    }
  }
}

// Task for parsing data from the serial monitor
void serial_parser(void *params){
  char local_buff[BUFF_LEN];
  char c;
  uint8_t idx = 0;

  memset(local_buff, 0, BUFF_LEN);

  // Task super-loop
  while(1){
    if(Serial.available() > 0){ // If data is available to be read
      c = Serial.read(); // Read byte

      // Values must be terminated with a newline char
      if(c != '\n' && idx < BUFF_LEN -1)local_buff[idx++] = c;
      else{ // Done reading, set the printing flag

        // Turn the buff into a string 
        /* 
         * We have to increment idx here (idx++) because later we'll use idx
         * to allocate bytes, and although for indexing we go 0 to n-1,
         * the number of bytes is n!
         */
        local_buff[idx++] = '\0';
        
        // Allocate memory on the heap for the other task
        buff = (char*)pvPortMalloc(idx * sizeof(char));

        configASSERT(buff);

        // Copy the read data to the heap buffer
        memcpy(buff, local_buff, idx);
        
        // Set the flag for the second task to allow printing
        can_print = 1;

        // Clear the buffer and reset index
        memset(local_buff, 0, BUFF_LEN);
        idx = 0; 
      }
    }
  }
}

// setup runs as its own tasks
void setup() {
  Serial.begin(115200);
  // Give time for the serial connection to be established
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  Serial.println("Write me something");
  
  // Task to run forever
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                serial_printer, // Function to run
                "Print ser",    // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                &print_,        // Task handle
                app_cpu);       // Run on one core

  // 2nd Task to run forever 
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                serial_parser,  // Function to run
                "Parse ser",    // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                &parse_,        // Task handle
                app_cpu);       // Run on one core
  
  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  
}
