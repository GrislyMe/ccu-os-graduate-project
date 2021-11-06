#!/usr/bin/env python3

import os
import pyimgur
import matplotlib.pyplot as plt
import webbrowser

def getCost(file_name: str):
    ret = [0] * 11
    idx = 0
    with open(file_name, "r") as inputFile:
        for idx, line in enumerate(inputFile.readlines()):
            ret[idx % 11] += int(line.split()[1])
    return [item / idx for item in ret]

def main():
    upload = False
    core_num = os.cpu_count()
    if not core_num:
        print("failed to get vcore_number")
        return

    x = ["160k", "120k", "80k", "40k", "20k", "10k", "5k", "2.5k", "1k", "0.5k", "0.1k"]
    xi = list(range(len(x)))
    y = getCost("../plock/plock_lps")
    line = plt.plot(xi, y, label = "plock")
    plt.setp(line, marker = "o")

    y = getCost("../ticket_lock/ticket_lps")
    line = plt.plot(xi, y, label = "ticket_lock")
    plt.setp(line, marker = "^")

    y = getCost("../SoA/SoA_lps")
    line = plt.plot(xi, y, label = "Soa_spinlock")
    plt.setp(line, marker = "s")

    y = getCost("../mcs/mcs_lps")
    line = plt.plot(xi, y, label = "mcs_spinlock")
    plt.setp(line, marker = "x")

    y = getCost("../plock/plock_lps.bak")
    line = plt.plot(xi, y, label = "plock_lps_old")
    plt.setp(line, marker = "v")

    plt.title("Lock Per Sec")
    plt.xticks(xi, x)
    plt.xlabel("rs_size")
    plt.ylabel("Lock Per Second")
    plt.grid(True)
    plt.legend()
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
