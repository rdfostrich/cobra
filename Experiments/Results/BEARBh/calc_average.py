import sys
import numpy as np
from itertools import islice
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 60})
plt.rcParams.update({'axes.linewidth' : 3})
def get_ms(ms):
    return ms/1000

def make_bar_plot(name, input):
    input = map(get_ms, input)
    fig, ax = plt.subplots(figsize=(12,14))
    x_pos = [1, 2]
    plt.bar(x_pos, input, width=0.9, color=['red', 'blue'], linewidth=2.0)
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Lookup (s)')
    #ax.set_title('Scores by group and gender')

    plt.xticks(x_pos, ("OSTRICH", "COBRA"))
    # ax.legend((rects1[0], rects2[0]), ('Men', 'Women'))
    # plt.show()
    plt.tight_layout()
    fig.savefig(name + ".png")

# MAIN

avg = open('average_per_triple_pattern.txt', 'r')

o_vm_avg_l = []
c_vm_avg_l = []
o_dm_avg_l = []
c_dm_avg_l = []
o_vq_avg_l = []
c_vq_avg_l = []

for line in avg:
    words = line.split(",")
    o_vm_avg_l.append(float(words[0]))
    c_vm_avg_l.append(float(words[1]))
    o_dm_avg_l.append(float(words[2]))
    c_dm_avg_l.append(float(words[3]))
    o_vq_avg_l.append(float(words[4]))
    c_vq_avg_l.append(float(words[5]))


make_bar_plot('avg_vm', [np.mean(o_vm_avg_l), np.mean(c_vm_avg_l)])
make_bar_plot('avg_dm', [np.mean(o_dm_avg_l), np.mean(c_dm_avg_l)])
make_bar_plot('avg_vq', [np.mean(o_vq_avg_l), np.mean(c_vq_avg_l)])

import os
os.remove('average_per_triple_pattern.txt')
