# -RTOS-communicating-tasks-project
The system contains 3 sender tasks and 1 receiver task which communicate through using a queue and binary semaphores which are used for synchronization between tasks, 2 sender tasks have the same priority and one has a higher priority each having a random time to execute while the receiver has a fixed time (100ms). Random time is controlled by 2 arrays for detecting the upper and lower bound. Bounds change every 1000 received messages till cover all bounds then terminate.
