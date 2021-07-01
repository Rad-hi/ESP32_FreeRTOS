# 7th lesson: Software timers and how to schedule events in FreeRTOS with them

Lesson: [https://www.youtube.com/watch?v=b1f1Iex0Tso&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=8](https://www.youtube.com/watch?v=b1f1Iex0Tso&list=PLEBQazB0HUyQ4hAPU1cJED6t3DU0h34bz&index=8)

In this lesson, the task was to create a task that would echo each char that's written to the Serial terminal while controlling a "backlight". The "backlight" is simulated with an LED that would turn on once a char has been entered to the serial port, and if nothing is entered for **5 seconds**, the LED must turn OFF. This functionality is illustrated in the first GIF down below.

I wanted to clean this up a bit so I added a fade-in and a fade-out to the LED, which is shown in the second GIF.

<img src="images/demo.gif" width=640>

> First GIF: No fading

<img src="images/demo_fade.gif" width=640>

> Second GIF: With fading
