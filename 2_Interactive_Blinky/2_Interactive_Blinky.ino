// Use only one core just for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
#define BLUE_LED_PIN    25
#define GREEN_LED_PIN   26

// Buffer length i.e. how many digits in the new delay time to have
#define BUFF_LEN       5

// Task handles which allow for inspection
static TaskHandle_t toggle = NULL;
static TaskHandle_t change = NULL;

// Blue LED blinking rate ms, this is the global variable we'll be using to change the blinking frequency
static int blink_rate = 500;

// Task for toggeling LED 
void toggle_led(void *params){
  // Task super-loop
  while(1){

    digitalWrite(BLUE_LED_PIN, HIGH);
    vTaskDelay(blink_rate / portTICK_PERIOD_MS); // Tick timer (non-blocking)

    digitalWrite(BLUE_LED_PIN, LOW);
    vTaskDelay(blink_rate / portTICK_PERIOD_MS);
  }
}

// Task for changing LED blinking rate
void change_rate(void *params){
  // Task super-loop
  char s[BUFF_LEN];
  char c;
  uint8_t idx = 0;

  memset(s, 0, BUFF_LEN);
  
  while(1){
    if(Serial.available() > 0){ // If data is available to be read
      c = Serial.read(); // Read byte

      // Values must be terminated with a newline char
      if(idx < BUFF_LEN && c != '\n'){
        
        // Append the value unless it's not a digit
        s[idx++] = isDigit(c) ? c : '\0'; 
      }
      else{
        // Done reading, convert the value to int and update the global blink_rate var
        blink_rate = strtol(s, NULL, 10);
        memset(s, 0, BUFF_LEN); // Clear the buffer
        idx = 0; // Reset the index
        Serial.printf("Blinking delay is now: %d\r\n", blink_rate);

        // Indicate change by flashing the green LED once
        digitalWrite(GREEN_LED_PIN, HIGH);
        vTaskDelay(200/ portTICK_PERIOD_MS);
        digitalWrite(GREEN_LED_PIN, LOW);
      }
    }
  }
}

// setup runs as its own tasks
void setup() {
  Serial.begin(115200);
  // Give time for the serial connection to be established
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Pin configuration
  pinMode(BLUE_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  
  // Task to run forever
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                toggle_led,     // Function to run
                "Toggle LED",   // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                &toggle,        // Task handle
                app_cpu);       // Run on one core

  // 2nd Task to run forever 
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                change_rate,    // Function to run
                "Change Rate",  // Name of task
                2024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                &change,        // Task handle
                app_cpu);       // Run on one core
}

void loop() {
  
}
