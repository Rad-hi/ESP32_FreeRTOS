// Use only core 1 for demo purposes
#if CONFIG_FREERTOS_UNICORE
  static const BaseType_t app_cpu = 0;
#else
  static const BaseType_t app_cpu = 1;
#endif

// Pins
#define LED_PIN         16

// PWM params (reference: https://randomnerdtutorials.com/esp32-pwm-arduino-ide/)
#define FREQUENCY       5000 // PWM signal frequency
#define LED_CHANNEL     0    // PWM channel
#define RESOLUTION      8    // Signal resolution in bits (8 --> [0, 255])

// Globals
static uint8_t led_state = 0;         // Flag for LED state
static TimerHandle_t timer_h = NULL;  // Timer handle

void timer_callback(TimerHandle_t timer_h){
  // Turn OFF LED
  led_state = 0;
}

// CLI
void CLI(void *parameters) {
  pinMode(LED_PIN, OUTPUT);

  // Var to control how many times to do the fading 
  // (obviously once, if configured to be more, there'll be a breathing effect)
  uint8_t once_ = 0;
  
  while(1){
    if(Serial.available()>0){
      // Read char and echo it back, note that this requires a serial terminal like putty)
      char c = Serial.read();
      Serial.print(c);

      // Turn ON LED and start counting
      led_state = 1;
      xTimerStart(timer_h, portMAX_DELAY);
    }
    if(led_state){
      // Once we fade-off once, the for loop will go from 0 to 0 since once_ == 1 
      for(int i = 0; i < (256 - 256*once_); i++){
        ledcWrite(LED_CHANNEL, i);
        vTaskDelay(2 / portTICK_PERIOD_MS);
      }
      once_ = 1;
    }
    else{
      // Once we fade-on once, the for loop will go from 0 to 0 since once_ == 0
      for(int i = 255 * once_; i >=0 ; i--){
        ledcWrite(LED_CHANNEL, i);
        vTaskDelay(2 / portTICK_PERIOD_MS);
      }
      once_ = 0;
    }
  }
  vTaskDelete(NULL);
}

void setup() {

  // Configure Serial
  Serial.begin(115200);

  // Wait a moment to start (so we don't miss Serial output)
  vTaskDelay(1000 / portTICK_PERIOD_MS);

  // Setup PWM channel
  ledcSetup(LED_CHANNEL, FREQUENCY, RESOLUTION);

  // Attach PWM channel to LED pin
  ledcAttachPin(LED_PIN, LED_CHANNEL);

  timer_h = xTimerCreate("Fade timer",              // Timer name
                         5000 / portTICK_PERIOD_MS, // Period of timer (in ticks)
                         pdTRUE,                    // Auto-reload
                         (void*)0,                  // Timer ID
                         timer_callback);           // Callback function

  if(timer_h == NULL) Serial.println("Couldn't create timer!");
  
  // CLI task creation
  xTaskCreatePinnedToCore(CLI,
                          "CLI",
                          1024,
                          NULL,
                          1,
                          NULL,
                          app_cpu);
                          
  Serial.println("Write me something!");
}

void loop() {
  
  // Do nothing but allow yielding to lower-priority tasks
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
