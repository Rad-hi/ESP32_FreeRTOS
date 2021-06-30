# 4th lesson: Using queues for task pipelining and arranging

Lesson: [https://www.youtube.com/watch?v=pHJ3lxOoWeI](https://www.youtube.com/watch?v=pHJ3lxOoWeI)

In this lesson, the task was to create two tasks that shall communicate through queues. The first task must act as a CLI (Command Line Interface) --> listen to the Serial port, and echo back any entered command. Additionally if this task, named **task A**, receives a command that starts with *delay* (I implemented it to also accept commands that contain *delay*) shall parse the numbers after it and pass them to the second task through a queue.

The 2nd task, named **task B** shall blink an LED, listen to the queue and if it receives a number, it would update the blinking delay accordingly. Additionally, if the LED blinks 100 times, **task B** must send the string "Blinked" to **task A**, and **task A** must echo the string "Blinked" to the command line.

To make things more fun, I added a 3rd task to act as a serial printing task. So this task would have a queue associated to it and whatever it finds in it, it'd print it to the CLI. This way, I have all printing happening inside its dedicated task.

<img src="images/demo.gif" width=640>

> running demo
