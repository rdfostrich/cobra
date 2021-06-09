#!/usr/bin/python

import sys
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

def process_pattern_lines(pattern_lines):
    return pattern_lines
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

def process_version_materialized_lines_avg(version_materialized_lines):
    version_materialized_lines= version_materialized_lines[2:]
    version_materialized_lines.sort(key = lambda x: int(x.split(",")[0]))
    # print(version_materialized_lines)
    count_time = []
    lookup_time = []
    for line in version_materialized_lines:
        # print(line)
        words = line.split(",")
        lookup_time.append(int(words[-2]))
        count_time.append(int(words[-3]))
    mean_count_time = int(np.mean(count_time))
    mean_lookup_time = int(np.mean(lookup_time))
    return (mean_count_time, mean_lookup_time)

def process_delta_materialized_lines(delta_materialized_lines):
    count_time = []
    lookup_time = []
    for line in delta_materialized_lines[2:]:
        # print(line)
        words = line.split(",")
        lookup_time.append(int(words[-2]))
        count_time.append(int(words[-3]))
    mean_count_time = int(np.mean(count_time))
    mean_lookup_time = int(np.mean(lookup_time))
    return (mean_count_time, mean_lookup_time)

def get_delta_materialized_version(delta_materialized_lines, version):
    count_time = []
    lookup_time = []

    for line in delta_materialized_lines[2:]:
        # print(line)
        words = line.split(",")
        start = int(words[0])
        end = int(words[1])
        if(start == version or end == version):
            lookup_time.append(int(words[-2]))
            count_time.append(int(words[-3]))
    return (count_time, lookup_time)


def process_detailed_delta_materialized_lines(delta_materialized_lines, snapshot):
    count_time = []
    lookup_time_case = []
    lookup_time_case_1 = []
    lookup_time_case_2 = []
    lookup_time_case_3 = []

    for line in delta_materialized_lines[2:]:
        # print(line)
        words = line.split(",")
        lookup_time.append(int(words[-2]))
        count_time.append(int(words[-3]))
        start = int(words[0])
        end = int(words[1])
        if(start == snapshot or end == snapshot): # case 1
            lookup_time_case_1.append(int(words[-2]))
        elif(start < snapshot and end > snapshot): # case 3
            lookup_time_case_3.append(int(words[-2]))
        else: # case 2
            lookup_time_case_2.append(int(words[-2]))

    mean_count_time = int(np.mean(count_time))
    mean_lookup_time = int(np.mean(lookup_time))
    mean_lookup_time_1 = int(np.mean(lookup_time_case_1))
    mean_lookup_time_2 = int(np.mean(lookup_time_case_2))
    mean_lookup_time_3 = int(np.mean(lookup_time_case_3))
    return (mean_count_time, mean_lookup_time, mean_lookup_time_1, mean_lookup_time_2, mean_lookup_time_3)

def process_version_lines(version_lines):
    count_time = []
    lookup_time = []
    for line in version_lines[2:]:
        # print(line)
        words = line.split(",")
        lookup_time.append(int(words[-2]))
        count_time.append(int(words[-3]))

    mean_count_time = int(np.mean(count_time))
    mean_lookup_time = int(np.mean(lookup_time))
    return (mean_count_time, mean_lookup_time)

def make_bar_plot(name, input):
    fig, ax = plt.subplots(figsize=(12,14))
    x_pos = [1, 2]
    plt.bar(x_pos, input, width=0.9, color=['red', 'blue'])
    # add some text for labels, title and axes ticks
    ax.set_ylabel('Lookup (ms)')
    #ax.set_title('Scores by group and gender')

    plt.xticks(x_pos, ("OSTRICH", "COBRA"))
    # ax.legend((rects1[0], rects2[0]), ('Men', 'Women'))
    # plt.show()
    plt.tight_layout()
    fig.savefig(name + ".png")

def make_line_plot(name, ostrich_l, cobra_l):
    fig = plt.figure(figsize=(12,12))
    plt.plot(np.arange(len(ostrich_l)), ostrich_l, '.r-', label='OSTRICH')
    plt.plot(np.arange(len(ostrich_l)), cobra_l, 'xb-', label='COBRA')
    plt.ylabel('Lookup (ms)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig(name + ".png", additional_artists=art, bbox_inches="tight")

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

o_vm_count_time_per_pattern_l = []
o_vm_res_time_per_pattern_l = []
o_dm_count_time_per_pattern_l = []
o_dm_res_time_per_pattern_l = []
o_vq_count_time_per_pattern_l = []
o_vq_res_time_per_pattern_l = []
c_vm_count_time_per_pattern_l = []
c_vm_res_time_per_pattern_l = []
c_dm_count_time_per_pattern_l = []
c_dm_res_time_per_pattern_l = []
c_vq_count_time_per_pattern_l = []
c_vq_res_time_per_pattern_l = []

while(True):
    pattern = next_n_lines(ostrich_query_file, 1)
    pattern = next_n_lines(cobra_query_file, 1)

    if(not pattern):
        break

    lines = next_n_lines(ostrich_query_file, 2 + number_of_versions)
    o_version_materialization_times = process_version_materialized_lines(lines, o_version_materialization_times)
    o_vm_count_time_per_pattern, o_vm_res_time_per_pattern = process_version_materialized_lines_avg(lines)
    lines = next_n_lines(ostrich_query_file, 2 + number_of_delta_lines)
    o_dm_count_time_l_per_pattern, o_dm_res_time_l_per_pattern = get_delta_materialized_version(lines,1)
    o_dm_count_time_per_pattern, o_dm_res_time_per_pattern = process_delta_materialized_lines(lines)

    o_vq_count_time_per_pattern, o_vq_res_time_per_pattern = process_version_lines(next_n_lines(ostrich_query_file, 2 + 1))

    lines = next_n_lines(cobra_query_file, 2 + number_of_versions)
    c_version_materialization_times = process_version_materialized_lines(lines,c_version_materialization_times)
    c_vm_count_time_per_pattern, c_vm_res_time_per_pattern = process_version_materialized_lines_avg(lines)
    lines = next_n_lines(cobra_query_file, 2 + number_of_delta_lines)
    c_dm_count_time_l_per_pattern, c_dm_res_time_l_per_pattern = get_delta_materialized_version(lines, 1)
    c_dm_count_time_per_pattern, c_dm_res_time_per_pattern = process_delta_materialized_lines(lines)
    c_vq_count_time_per_pattern, c_vq_res_time_per_pattern = process_version_lines(next_n_lines(cobra_query_file, 2 + 1))

    # output_query_file.write('\'' + str(pattern[0][18:-2]) + '\'' + ',' +
    #     str(vm_count_time_per_pattern) + ',' +
    #     str(vm_res_time_per_pattern) + ',' +
    #     str(dm_count_time_per_pattern) + ',' +
    #     str(dm_res_time_per_pattern) + ',' +
    #     str(vq_count_time_per_pattern) + ',' +
    #     str(vq_res_time_per_pattern) + '\n')

    o_vm_res_time_per_pattern_l.append(o_vm_res_time_per_pattern)
    # o_dm_count_time_per_pattern_l.append(o_dm_count_time_per_pattern)
    o_dm_res_time_per_pattern_l.append(o_dm_res_time_per_pattern)
    # o_vq_count_time_per_pattern_l.append(o_vq_count_time_per_pattern)
    o_vq_res_time_per_pattern_l.append(o_vq_res_time_per_pattern)

    c_vm_res_time_per_pattern_l.append(c_vm_res_time_per_pattern)
    # c_dm_count_time_per_pattern_l.append(c_dm_count_time_per_pattern)
    c_dm_res_time_per_pattern_l.append(c_dm_res_time_per_pattern)
    # c_vq_count_time_per_pattern_l.append(c_vq_count_time_per_pattern)
    c_vq_res_time_per_pattern_l.append(c_vq_res_time_per_pattern)

ostrich_query_file.close()
cobra_query_file.close()

# make_line_plot(output_dir + "vm_count_" + triple_pattern, o_vm_count_time_per_pattern_l, c_vm_count_time_per_pattern_l)
make_line_plot(input_dir + "dm_res_" + triple_pattern, o_dm_res_time_l_per_pattern,c_dm_res_time_l_per_pattern)
make_line_plot(input_dir + "vm_res_" + triple_pattern, np.mean(o_version_materialization_times, axis=1), np.mean(c_version_materialization_times, axis=1))

make_bar_plot(input_dir + "vm_res_bar_" + triple_pattern, [np.mean(o_vm_res_time_per_pattern_l), np.mean(c_vm_res_time_per_pattern_l)])
# make_bar_plot(output_dir + "dm_count_" + triple_pattern, [np.mean(o_dm_count_time_per_pattern_l), np.mean(c_dm_count_time_per_pattern_l)])
make_bar_plot(input_dir + "dm_res_" + triple_pattern, [np.mean(o_dm_res_time_per_pattern_l), np.mean(c_dm_res_time_per_pattern_l)])
# make_bar_plot(output_dir + "vq_count_" + triple_pattern, [np.mean(o_vq_count_time_per_pattern_l), np.mean(c_vq_count_time_per_pattern_l)])
make_bar_plot(input_dir + "vq_res_" + triple_pattern, [np.mean(o_vq_res_time_per_pattern_l), np.mean(c_vq_res_time_per_pattern_l)])

#write averages
avg = open(input_dir + 'average_per_triple_pattern.txt', 'a')
avg.write(
    str(np.mean(o_vm_res_time_per_pattern_l)) + "," +
    str(np.mean(c_vm_res_time_per_pattern_l)) + "," +
    str(np.mean(o_dm_res_time_per_pattern_l)) + "," +
    str(np.mean(c_dm_res_time_per_pattern_l)) + "," +
    str(np.mean(o_vq_res_time_per_pattern_l)) + "," +
    str(np.mean(c_vq_res_time_per_pattern_l)) + '\n'
    )
