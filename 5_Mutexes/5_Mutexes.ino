// Use only one core just for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
#define LED_PIN         25

// FreeRTOS takes advantage of the fact that semaphores and mutexes are similar
// and hence uses semaphores handles for both semaphores and mutexes
static SemaphoreHandle_t mutex;

// Blinking Task
void blink_led(void *params){
  int local;

  local = *(int*)params;
  // Copy the param to a local var
  Serial.printf("Received: %d\r\n", local);

  pinMode(LED_PIN, OUTPUT);
  
  // Super-loop
  while(1){
    digitalWrite(LED_PIN, 1);
    vTaskDelay(local / portTICK_PERIOD_MS);
    digitalWrite(LED_PIN, 0);
    vTaskDelay(local / portTICK_PERIOD_MS);
  }

  // Give the mutex (make it available)
  xSemaphoreGive(mutex);
}

void setup() {
  
  // Local delay
  long int delay_in = 200;
  
  Serial.begin(115200);
  // Give time for the serial connection to be established
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Create the mutex and assign it to the global handle
  mutex = xSemaphoreCreateMutex();
  
  Serial.println("Enter a delay(milliseconds)");

  // Wait for serial input
  while(Serial.available() <= 0);
  
  // Read the input
  delay_in = Serial.parseInt();
  Serial.printf("Seding: %d\r\n", delay_in);

  // Attempt to take the mutex in a blocking manner 
  // (wait the maximum time possible;portMAX_DELAY)
  xSemaphoreTake(mutex, portMAX_DELAY);

  // 1st Task to run forever --> Serial printer
  xTaskCreatePinnedToCore(        // xTaskCreate() in vanilla FreeRTOS
                blink_led,        // Function to run
                "PBlinky",        // Name of task
                1024,             // Stack size (bytes in ESP32, words in FreeRTOS)
                (void*)&delay_in, // Params to pass to function
                1,                // Task priority (0 to configMAX_PRIORITIES -1)
                NULL,             // Task handle
                app_cpu);         // Run on one core

  // Wait till the mutex is returned the maximum time possible
  if(xSemaphoreTake(mutex, portMAX_DELAY) == pdTRUE)Serial.println("Done!");
                
  // Delete "setup and loop" task
  vTaskDelete(NULL);
}

void loop() {
  // Nothing 
}
