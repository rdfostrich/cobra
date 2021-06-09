import numpy as np
import os
import pickle
import matplotlib.pyplot as plt
import math
plt.rcParams.update({'font.size': 60})
plt.rcParams.update({'axes.linewidth' : 3})
plt.rcParams.update({'lines.linewidth' : 3})
plt.rcParams.update({'lines.markersize' : 10})
def make_line_plot(name, ostrich_l, cobra_l):
    fig = plt.figure(figsize=(12,12))
    plt.plot(np.arange(len(ostrich_l)), ostrich_l, '.r-', label='OSTRICH',markevery=int(math.ceil(len(ostrich_l)/20.0)), linewidth=2.0)
    plt.plot(np.arange(len(ostrich_l)), cobra_l, 'xb-', label='COBRA', markevery=int(math.ceil(len(ostrich_l)/20.0)), linewidth=2.0)
    plt.ylabel('Lookup (ms)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig(name + ".png", additional_artists=art, bbox_inches="tight")

o_vm_l = []
c_vm_l = []
o_dm_l = []
c_dm_l = []

#read averages
with open('o_vm_average_l.txt','rb') as f:
    while(True):
        try:
            o_vm_l.append(pickle.load(f))
        except (EOFError):
            break
with open('c_vm_average_l.txt','rb') as f:
    while(True):
        try:
            c_vm_l.append(pickle.load(f))
        except (EOFError):
            break
with open('o_dm_average_l.txt','rb') as f:
    while(True):
        try:
            o_dm_l.append(pickle.load(f))
        except (EOFError):
            break
with open('c_dm_average_l.txt','rb') as f:
    while(True):
        try:
            c_dm_l.append(pickle.load(f))
        except (EOFError):
            break

o_vm_l = [map(float, l) for l in o_vm_l]
c_vm_l = [map(float, l) for l in c_vm_l]
o_dm_l = [map(float, l) for l in o_dm_l]
c_dm_l = [map(float, l) for l in c_dm_l]

make_line_plot('vm_mean_line', np.mean(o_vm_l, axis=0), np.mean(c_vm_l, axis=0))
make_line_plot('dm_mean_line', np.mean(o_dm_l, axis=0), np.mean(c_dm_l, axis=0))

os.remove('o_vm_average_l.txt')
os.remove('c_vm_average_l.txt')
os.remove('o_dm_average_l.txt')
os.remove('c_dm_average_l.txt')
