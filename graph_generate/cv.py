#!/usr/bin/env python3
# cv = σ/μ
# σ = sqrt((sum(Xi - μ))^2/amount)
# μ = average
import numpy as np
import matplotlib.pyplot as plt

def cv(filename: str):
    # 160000, 120000, 80000, 40000, 20000, 10000, 5000, 2500, 1000, 500, 100
    dt_amount = 11
    count = 0
    sum = np.zeros(11)
    avg = np.zeros(11)
    S_D = np.zeros(11)
    CV = np.zeros(11)

    # calculate the average
    with open(filename, 'r') as f:
        for line in f:
            parse = line.split()
            value = (int)(parse[1])
            sum[count % dt_amount] += value
            count += 1

    times = count / dt_amount
    for i in range(dt_amount):
        avg[i] = sum[i] / times

    # calculate the standard deriviation
    count = 0
    with open('../SoA/SoA_lps', 'r') as f:
        for line in f:
            parse = line.split()
            index = count % dt_amount
            value = (int)(parse[1])
            S_D[index] += ((value - avg[index]) ** 2)
            count += 1

    for i in range(dt_amount):
        S_D[i] /= times
        S_D[i] = np.sqrt(S_D[i])
    for i in range(dt_amount):
        print(i, "\t", S_D[i], "\t", avg[i])

    # calculate CV
    for i in range(dt_amount):
        CV[i] = (S_D[i] / avg[i]) * 100

    return CV


def main():
    x_axis = ["160k", "120k", "80k", "40k", "20k", "10k", "5k", "2.5k", "1k", "0.5k", "0.1k"]

    SoA_CV = cv('../SoA/SoA_lps')
    plock_CV = cv('../plock/plock_lps')
    MCS_CV = cv('../mcs/mcs_lps')
    tlock_CV = cv('../ticket_lock/ticket_lps')

    #plt.figure(figsize = (15, 10), dpi = 100, linewidth = 2)
    plt.plot(x_axis, SoA_CV, 's-', color = 'r', label = "SoA")
    plt.plot(x_axis, plock_CV, 'o-', color = 'g', label = "plock")
    plt.plot(x_axis, MCS_CV, 'x-', color = 'b', label = "MCS")
    plt.plot(x_axis, tlock_CV, '^-', color = 'y', label = "tlock")

    plt.title('coefficient of variation (CV)')
    plt.xlabel('Remainder Section Size', fontsize = 30, labelpad = 15)
    plt.legend(loc = "best", fontsize = 20)
    plt.grid()
    plt.show()

if __name__ == '__main__':
    main()
