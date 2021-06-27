// Use only one core just for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
#define LED_PIN 2

// Task for toggeling LED 
void toggle_led(void *params){
  // Task super-loop
  while(1){
    Serial.println("ON");
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(500 / portTICK_PERIOD_MS); // Tick timer (non-blocking)

    Serial.println("OFF");
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }
}

// Task for ruining toggeling LED rates
void toggle_diff(void *params){
  // Task super-loop
  while(1){
    Serial.println("ON");
    digitalWrite(LED_PIN, HIGH);
    vTaskDelay(200 / portTICK_PERIOD_MS); // Tick timer (non-blocking)

    Serial.println("OFF");
    digitalWrite(LED_PIN, LOW);
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

// setup runs as its own tasks
void setup() {
  Serial.begin(115200);

  // Pin configuration
  pinMode(LED_PIN, OUTPUT);

  // Task to run forever
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                toggle_led,     // Function to run
                "Toggle LED",   // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,           // Task handle
                app_cpu);       // Run on one core

  // 2nd Task to run forever 
  // (this should interrupt the other task's blinking pattern and introduce 
  // weird blinking patterns)
  xTaskCreatePinnedToCore(      // xTaskCreate() in vanilla FreeRTOS
                toggle_diff,    // Function to run
                "Toggle LED",   // Name of task
                1024,           // Stack size (bytes in ESP32, words in FreeRTOS)
                NULL,           // Params to pass to function
                1,              // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,           // Task handle
                app_cpu);       // Run on one core
}

void loop() {
  
}
