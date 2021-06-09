#!/usr/bin/python

import sys
import pickle
import math
import numpy as np
from itertools import islice
import matplotlib.pyplot as plt
plt.rcParams.update({'font.size': 60})
plt.rcParams.update({'axes.linewidth' : 3})
plt.rcParams.update({'lines.linewidth' : 3})
plt.rcParams.update({'lines.markersize' : 10})


def next_n_lines(file_opened, N):
    return [x.strip() for x in islice(file_opened, N)]

def calc_sum(n):
    res = 0;
    for i in range(n-1, 0, -1):
        res += i;
    return res;


def make_line_plot(name, ostrich_l, cobra_l):
    fig = plt.figure(figsize=(12,12))
    plt.plot(np.arange(len(ostrich_l)), ostrich_l, '.r-', label='OSTRICH',markevery=int(math.ceil(len(ostrich_l)/20.0)))
    plt.plot(np.arange(len(ostrich_l)), cobra_l, 'xb-', label='COBRA', markevery=int(math.ceil(len(ostrich_l)/20.0)))
    plt.ylabel('Lookup (ms)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig(name + ".png", additional_artists=art, bbox_inches="tight")

def process_version_materialized_lines(version_materialized_lines, version_materialization_times):
    version_materialized_lines= version_materialized_lines[2:]

    version_materialized_lines.sort(key = lambda x: int(x.split(",")[0]))
    # print(version_materialized_lines)

    count_time = []
    lookup_time = []
    version = 0

    for line in version_materialized_lines:
        words = line.split(",")
        version_materialization_times[version].append(int(words[-2]))
        version += 1
    return version_materialization_times

def process_delta_materialized_lines(delta_materialized_lines, delta_materialization_times, version):
    delta_materialized_lines= delta_materialized_lines[2:]
    count_time = []
    lookup_time = []
    for line in delta_materialized_lines:
        words = line.split(",")
        start = int(words[0])
        end = int(words[1])

        if(version == start):
            delta_materialization_times[end].append(int(words[-2]))
        elif(version==end):
            delta_materialization_times[start].append(int(words[-2]))
    delta_materialization_times[version].append(0)
    return delta_materialization_times

# MAIN
if(len(sys.argv)!=4):
    print "Not enough arguments, make sure to provide path_ostrich_query_file, path_cobra_query_file, number_of_versions"
    sys.exit(-1)

path_ostrich_query_file = str(sys.argv[1]);
path_cobra_query_file = str(sys.argv[2]);
number_of_versions = int(sys.argv[3]);
print("path_ostrich_query_file: " + path_ostrich_query_file)
print("path_cobra_query_file: " + path_cobra_query_file)
print("number_of_versions: " + str(number_of_versions))

triple_pattern = path_ostrich_query_file[path_ostrich_query_file.find("ostrich/")+len("ostrich/"):path_ostrich_query_file.rfind("-query.txt")]
input_dir = path_ostrich_query_file[0:path_ostrich_query_file.find("ostrich/")]

ostrich_query_file = open(path_ostrich_query_file, 'r')
cobra_query_file = open(path_cobra_query_file, 'r')
number_of_delta_lines = calc_sum(number_of_versions)


o_version_materialization_times = [[] for i in range(number_of_versions)]
c_version_materialization_times = [[] for i in range(number_of_versions)]

o_delta_materialization_times = [[] for i in range(number_of_versions)]
c_delta_materialization_times = [[] for i in range(number_of_versions)]

dm_version = 3

while(True):
    pattern = next_n_lines(ostrich_query_file, 1)
    pattern = next_n_lines(cobra_query_file, 1)

    if(not pattern):
        break

    o_vm_lines = next_n_lines(ostrich_query_file, 2 + number_of_versions)
    o_dm_lines = next_n_lines(ostrich_query_file, 2 + number_of_delta_lines)
    o_vq_lines = next_n_lines(ostrich_query_file, 2 + 1)

    c_vm_lines = next_n_lines(cobra_query_file, 2 + number_of_versions)
    c_dm_lines = next_n_lines(cobra_query_file, 2 + number_of_delta_lines)
    c_vq_lines = next_n_lines(cobra_query_file, 2 + 1)

    o_version_materialization_times = process_version_materialized_lines(o_vm_lines, o_version_materialization_times)
    c_version_materialization_times = process_version_materialized_lines(c_vm_lines, c_version_materialization_times)

    o_delta_materialization_times = process_delta_materialized_lines(o_dm_lines, o_delta_materialization_times, dm_version)
    c_delta_materialization_times = process_delta_materialized_lines(c_dm_lines, c_delta_materialization_times, dm_version)

ostrich_query_file.close()
cobra_query_file.close()

make_line_plot(triple_pattern + "_vm_res_" + triple_pattern, np.mean(o_version_materialization_times, axis=1), np.mean(c_version_materialization_times, axis=1))
make_line_plot(triple_pattern + "_line_dm_res_" + triple_pattern, np.mean(o_delta_materialization_times, axis=1), np.mean(c_delta_materialization_times, axis=1))

#write averages
with open('o_vm_average_l.txt','ab') as f:
    pickle.dump(map(str, np.mean(o_version_materialization_times, axis=1)), f)
with open('c_vm_average_l.txt','ab') as f:
    pickle.dump(map(str, np.mean(c_version_materialization_times, axis=1)), f)
with open('o_dm_average_l.txt','ab') as f:
    pickle.dump(map(str, np.mean(o_delta_materialization_times, axis=1)), f)
with open('c_dm_average_l.txt','ab') as f:
    pickle.dump(map(str, np.mean(c_delta_materialization_times, axis=1)), f)
