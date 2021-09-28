#!/usr/bin/env python3

import os
import pyimgur
import numpy as np
import matplotlib.pyplot as plt
import webbrowser

def getCost():
    ret = []
    merge = 2
    with open("cost", "r") as inputFile:
        for line in inputFile.readlines():
            ret += [list(map(int, line.split()[1:]))]
    return np.array([np.sum(np.array(ret)[i - merge: i], axis=0) for i in range(merge, 100, merge)])

def main():
    upload = False
    core_num = os.cpu_count()
    if not core_num:
        print("failed to get vcore_number")
        return

    cost = getCost();
    #plt.figure(figsize=(24,4))

    plt.imshow(cost)

    plt.tick_params(labelleft=False, labelright=False,
                   labeltop=False, labelbottom=False,
                   left=False, right=False,
                   top=False, bottom=False)

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
