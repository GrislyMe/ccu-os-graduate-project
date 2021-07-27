#!/usr/bin/env python3

import os
import pyimgur
import numpy as np
import matplotlib.pyplot as plt
import webbrowser

core_num = os.cpu_count()

def getTime():
    count = np.zeros(shape=(core_num, core_num))
    time = np.zeros(shape=(core_num, core_num))
    with open("./result", 'r') as output:
        for i in output.readlines():
            t, c, s, d = map(int, i.split())
            
            if(s >= 0 and d >= 0):
                time[s][d] += t
                count[s][d] += c
    
    return time // count

def main():
    cpu = list(range(core_num))

    time = getTime()

    fig, ax = plt.subplots()
    im = ax.imshow(time)

    # We want to show all ticks...
    ax.set_xticks(np.arange(core_num))
    ax.set_yticks(np.arange(core_num))
    # ... and label them with the respective list entries
    ax.set_xticklabels(cpu)
    ax.set_yticklabels(cpu)

    # Rotate the tick labels and set their alignment.
    ax.tick_params(top=True, bottom=False,
                   labeltop=True, labelbottom=False)
    plt.setp(ax.get_xticklabels(), ha="right",
             rotation_mode="anchor")

    # Loop over data dimensions and create text annotations.
    textcolors = ['w', 'k']
    norm = im.norm(time)
    for i in range(core_num):
        for j in range(core_num):
            text = ax.text(j, i, int(time[i, j]),
                           ha="center", va="center", 
                           color=textcolors[int(norm[i, j] > 0.5)], size=4)

    ax.set_title("context switch time")
    fig.tight_layout()
    plt.savefig("/tmp/result.png", dpi=350)

    CLIENT_ID = "2964f9789cd9671"
    im = pyimgur.Imgur(CLIENT_ID)
    image = im.upload_image("/tmp/result.png")
    webbrowser.open(image.link, new=2)
    print(image.link)

if __name__ == "__main__":
    main()
