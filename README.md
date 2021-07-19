# CCU finalProject

## CURRENT STATUS

Using pthread_spinlock in each thread to secure the critical section.
In the critical section, we perform some floating point arithmetic, and record the time cost by swithes in and the time cost by switches out.
Then we use these time records to calculate the total switching time.

## ENVIRONMENT

Currently we are running those test on mainly two machine
1. Ryzen 3500x
2. Ryzen 4750u

The lstopo graph on Ryzen 4750u
![](https://i.imgur.com/9wZs163.png)

The lstopo graph on Ryzen 3500x
![](https://i.imgur.com/fQe1O73.png)



## RESULT

each element of matrix is average of 50k times switches
Perform on Ryzen 4750u
![](https://cdn.discordapp.com/attachments/824603223057891339/866594814282432562/Figure_3.png)

Perform on Ryzen 3500x

![](https://media.discordapp.net/attachments/824603223057891339/866602853194661898/2021-07-19_16-50-00_.png)

![](https://i.imgur.com/IAB7lIk.png)
