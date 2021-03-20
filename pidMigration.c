#define _GNU_SOURCE

#include <stdio.h>
#include <sched.h>
#include <time.h>

// set this number according to you machine
int maxCpu = 8;

int setCpu(int id) {
    // change this process to desire cpu
    // return 0 if sucess
    if(id > maxCpu)
        return -1;
    cpu_set_t cpu;
    CPU_ZERO(&cpu);
    CPU_SET(id, &cpu);
    if(sched_setaffinity(0, sizeof(cpu), &cpu) == -1)
        return -1;
    return 0;
}

int main(void) {
    clock_t t;

    for(int src = 0; src < 8; src++) {
        for(int dst = 0; dst < 8; dst++) {
            setCpu(src);
            // set cpu affinity to source core
            t = clock();
            // init clock
            setCpu(dst);
            // set cpu affinity to destination core
            t = clock() - t;
            // this line I'm not so sure
            // in my imagination this line will be run after the cpu migration is done

            // double takes = ((double)t) / CLOCKS_PER_SEC;
            // printf("%d to %d takes %lf\n", src, dst, takes);
            printf("%d to %d takes %d\n", src, dst, t);
        }
    }
}
