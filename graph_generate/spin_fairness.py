#!/usr/bin/env python3

import os
import pyimgur
import numpy as np
import matplotlib.pyplot as plt
import webbrowser

def getCost(file_name: str):
    ret = []
    merge = 4
    with open(file_name, "r") as inputFile:
        for line in inputFile.readlines():
            ret += [list(map(int, line.split()[1:]))]
    return np.array([np.sum(np.array(ret)[i - merge: i], axis=0) for i in range(merge, 100, merge)])

def main():
    upload = False
    core_num = os.cpu_count()
    if not core_num:
        print("failed to get vcore_number")
        return



    fig, ax = plt.subplots(1, 3)

    ax[0].imshow(getCost("../plock/plock_cost"), label = "plock")
    ax[0].tick_params(labelleft=False, labelright=False,
                   labeltop=False, labelbottom=False,
                   left=False, right=False,
                   top=False, bottom=False)
    ax[0].set_title("Plock")

    ax[1].imshow(getCost("../ticket_lock/ticket_cost"))
    ax[1].tick_params(labelleft=False, labelright=False,
                   labeltop=False, labelbottom=False,
                   left=False, right=False,
                   top=False, bottom=False)
    ax[1].set_title("Ticket Lock")

    ax[2].imshow(getCost("../mcs/MCS_cost"))
    ax[2].tick_params(labelleft=False, labelright=False,
                   labeltop=False, labelbottom=False,
                   left=False, right=False,
                   top=False, bottom=False)
    ax[2].set_title("MCS Lock")

    fig.suptitle('Fairness Test',fontsize=16) 
    plt.show()

    if upload:
        plt.savefig("/tmp/result.png", dpi=350)
        CLIENT_ID = "2964f9789cd9671"
        im = pyimgur.Imgur(CLIENT_ID)
        image = im.upload_image("/tmp/result.png")
        webbrowser.open(image.link, new=2)
        print(image.link)

if __name__ == "__main__":
    main()
