#!/usr/bin/env python3

import sys
import numpy as np
import matplotlib
import matplotlib.pyplot as plt
import subprocess

thread_num = 16

def getTime():
    testRound = 1
    args = ("./while_test.o")
    time = np.array([[0 * i for i in range(thread_num)]] * thread_num)
    with open("./result", 'r') as output:
        for i in output.readlines():
            tmp = i.split()
            s = int(tmp[8])
            d = int(tmp[10])
            #print(tmp, s, d)
            if(s >= 0 and d >= 0):
                # if(int(tmp[3]) > 3000):
                #    continue
                if(time[s, d] == 0):
                    time[s, d] += int(tmp[3])
                else:
                    time[s, d] += int(tmp[3])
                    time[s, d] /= 2;
                #print(time[s, d])
        
    return time



def main():
    cpu = [i for i in range(thread_num)]

    time = getTime()

    fig, ax = plt.subplots()
    im = ax.imshow(time)

    # We want to show all ticks...
    ax.set_xticks(np.arange(len(cpu)))
    ax.set_yticks(np.arange(len(cpu)))
    # ... and label them with the respective list entries
    ax.set_xticklabels(cpu)
    ax.set_yticklabels(cpu)

    # Rotate the tick labels and set their alignment.
    ax.tick_params(top=True, bottom=False,
                   labeltop=True, labelbottom=False)
    plt.setp(ax.get_xticklabels(), ha="right",
             rotation_mode="anchor")

    # Loop over data dimensions and create text annotations.
    for i in range(len(cpu)):
        for j in range(len(cpu)):
            text = ax.text(j, i, int(time[i, j]),
                           ha="center", va="center", color="w")

    ax.set_title("context switch time")
    fig.tight_layout()
    plt.show()

if __name__ == "__main__":
    main()
