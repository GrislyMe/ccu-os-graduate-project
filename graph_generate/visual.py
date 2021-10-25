#!/usr/bin/env python3
import os
import pyimgur
import numpy as np
import matplotlib.pyplot as plt
import webbrowser
import TSP_order

core_num = os.cpu_count()

def getTime(core_num):
    count = np.zeros(shape=(core_num, core_num))
    time = np.zeros(shape=(core_num, core_num))
    with open("../misc/result", 'r') as output:
        for i in output.readlines():
            t, c, s, d = map(int, i.split())

            time[s][d] += t
            count[s][d] += c
    return time // count

def toIDCov(route: str, ccx_num: int):
    core_in_ccx = 8
    r = list(map(int, route.split(",")))
    ret = []
    for t in range(1, 1 + ccx_num):
        tmp = [0] * core_in_ccx
        ccx = [core for core in r[: -1] if (t - 1) * core_in_ccx <= core < t * core_in_ccx]
        if not t % 2:
            ccx.reverse()
        pre = ccx[0]
        ccx += [ccx[0]]

        for i in ccx[1:]:
            tmp[pre % core_in_ccx] = i
            pre = i
        ret += tmp

    return ret

def main():
    upload = 0
    if not core_num:
        print("failed to get vcore_number")
        return
    cpu = list(range(core_num))

    time = np.nan_to_num(getTime(core_num))

    fig, ax = plt.subplots()
    im = ax.imshow(time)

    # We want to show all ticks...
    ax.set_xticks(range(core_num))
    ax.set_yticks(range(core_num))
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

    ax.set_title("context switch time")
    fig.tight_layout()

    if upload:
        for i in range(core_num):
            for j in range(core_num):
                ax.text(j, i, int(time[i, j]),
                               ha="center", va="center",
                               color=textcolors[int(norm[i, j] > 0.5)], size=4)
        plt.savefig("/tmp/result.png", dpi=350)
        CLIENT_ID = "2964f9789cd9671"
        im = pyimgur.Imgur(CLIENT_ID)
        image = im.upload_image("/tmp/result.png")
        webbrowser.open(image.link, new=2)
        print(image.link)
    else:
        #for i in range(core_num):
        #    for j in range(core_num):
        #        ax.text(j, i, int(time[i, j]),
        #                       ha="center", va="center",
        #                       color=textcolors[int(norm[i, j] > 0.5)])
        plt.show()

    route = TSP_order.main(time, 1)
    if(route):
        print(route)
        print(toIDCov(route[6:], 2))
    else:
        print("fail to find route")


if __name__ == "__main__":
    main()
