# CCU finalProject

## CURRENT STATUS

Using pthread_spinlock in each thread to secure the critical section.
In the critical section, we perform some floating point arithmetic, and record the time cost by swithes in and the time cost by switches out.
Then we use these time records to calculate the total switching time.


In the beginning, we create threads to test the CPU's core swithcing overhead of communication.

In each thread, it will access the global data to do each core's data communication. And we made it a for-loop to access about 100 times, which will make the amount of time cost larger, so the result would be more easier for us to check.

Then, we record the finish time of accessing the global data, and check how long the time that the thread changing and data accessing costs.

Time checking and data accessing are placed in C.S. which is lock by pthread_spin_lock().

Each thread will repeat 100 times to compute the average time cost.

## ENVIRONMENT

Currently we are running those test on these listed machine
1. Ryzen 3500x
2. Ryzen 4750u
3. i7-4790

```shell=
lstopo -l --ignore pci --ignore block --of png > lstopo.png
```

The lstopo graph on Ryzen 3500x

![](https://i.imgur.com/VtZdhwk.png)

The lstopo graph on Ryzen 4750u

![](https://i.imgur.com/xDSDeAx.png)

The lstopo graph on i7-4790

![](https://i.imgur.com/eSlJXni.png)

## STEP

1. using [test.sh](https://github.com/GrislyMe/ccu-os-graduate-project/blob/main/test.sh) to run the test
```shell=
./test.sh
```

## RESULT

each element of matrix is average of 10k times switches

Perform on Ryzen 4750u
![](https://i.imgur.com/AFJCCnY.png)

Perform on Ryzen 3500x
![](https://i.imgur.com/qtQNPSS.png)

Perform on i7-4790
![](https://i.imgur.com/y84MfZO.png)

