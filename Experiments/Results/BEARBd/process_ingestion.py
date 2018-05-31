import sys
import numpy as np
from itertools import islice
import matplotlib.pyplot as plt
from itertools import izip

plt.rcParams.update({'font.size': 60})
plt.rcParams.update({'axes.linewidth' : 3})
plt.rcParams.update({'lines.linewidth' : 3})
plt.rcParams.update({'lines.markersize' : 10})

def get_K(n):
    return n/1e3
def get_min(ms):
    return ms / 6e4
def get_MB(B):
    return B / 1e6

def process_file(file):
    storage_l = []
    duration_l = []
    duration_per_version_l = []
    added_l = []
    version_l = []

    for line in file:
        words = line.split(",")
        words2 = line.split(" ")
        if(len(words2) < 3):
            # version,added,duration,rate,accsize
            version = int(words[0])
            added = int(words[1])
            duration = int(words[2])
            accsize = int(words[4])
            storage_l.append(accsize)
            duration_l.append(( 0 if not duration_l else duration_l[-1]) + duration) #cum sum
            added_l.append((0 if not added_l else added_l[-1]) + added) #cum sum
            version_l.append(version)
            duration_per_version_l.append(duration)
        else:
            duration = int(words2[-1])
            duration_l.append(( 0 if not duration_l else duration_l[-1]) + duration) #cum sum
            duration_per_version_l.append(duration)

    return (version_l, added_l, duration_l, storage_l, duration_per_version_l)

def process_pre_fix_up_file(file):
    storage_l = []
    duration_l = []
    duration_per_version_l = []
    added_l = []
    version_l = []

    for line in file:
        words = line.split(",")
        # version,added,duration,rate,accsize
        version = int(words[0])
        added = int(words[1])
        duration = int(words[2])
        accsize = int(words[4])
        if(len(version_l) == version + 1): # double entry for a version
            print(version)
            storage_l[version] = accsize
            duration_l[version] += duration
            added_l[version] += added
            duration_per_version_l[version] += duration
        else:
            storage_l.append(accsize)
            duration_l.append(( 0 if not duration_l else duration_l[-1]) + duration) #cum sum
            added_l.append((0 if not added_l else added_l[-1]) + added) #cum sum
            version_l.append(version)
            duration_per_version_l.append(duration)

    return (version_l, added_l, duration_l, storage_l, duration_per_version_l)

def make_line_plot_ingestion_time(ostrich_l, ostrich_opt_l, cobra_pre_l, cobra_l):
    fig = plt.figure(figsize=(12,12))
    plt.plot(ostrich_l[0], map(get_min, ostrich_l[4]), '.-', color ="red", label='OSTRICH-1F',markevery=5)
    plt.plot(ostrich_opt_l[0], map(get_min, ostrich_opt_l[4]), '>-', color = "orange", label='OSTRICH-2F',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_min, cobra_pre_l[4]), 'x-', color = "royalblue", label='COBRA-PRE_FIX_UP',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_min, cobra_l[4]), '^-', color ="b", label='COBRA-OUT_OF_ORDER',markevery=5)
    plt.ylabel('Duration (min)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig("per_version_ingestion_plot.png", additional_artists=art, bbox_inches="tight")

def make_line_plot_total_ingestion_time(ostrich_l, ostrich_opt_l, cobra_pre_l, cobra_l):
    fig = plt.figure(figsize=(12,12))
    plt.plot(ostrich_l[0], map(get_min, ostrich_l[2]), '.-', color ="red", label='OSTRICH-1F',markevery=5)
    plt.plot(ostrich_opt_l[0], map(get_min, ostrich_opt_l[2]), '>-', color = "orange", label='OSTRICH-2F',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_min, cobra_pre_l[2]), 'x-', color = "royalblue", label='COBRA-PRE_FIX_UP',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_min, cobra_l[2]), '^-', color ="b", label='COBRA-OUT_OF_ORDER',markevery=5)
    plt.ylabel('Duration (min)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig("ingestion_plot.png", additional_artists=art, bbox_inches="tight")

def make_line_plot_storage_size(ostrich_l, ostrich_opt_l, cobra_pre_l, cobra_l):

    fig = plt.figure(figsize=(12,12))
    plt.plot(ostrich_l[0], map(get_MB, ostrich_l[3]), '.-', color ='r', label='OSTRICH-1F',markevery=5)
    plt.plot(ostrich_opt_l[0], map(get_MB, ostrich_opt_l[3]), '>-', color = 'orange', label='OSTRICH-2F',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_MB, cobra_pre_l[3]), 'x-', color = 'royalblue', label='COBRA-PRE_FIX_UP',markevery=5)
    plt.plot(cobra_pre_l[0], map(get_MB, cobra_l[3]), '^-', color ='b', label='COBRA-OUT_OF_ORDER',markevery=5)
    plt.ylabel('Size (MB)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig("storage_plot.png", additional_artists=art, bbox_inches="tight")

def make_line_plot_added(ostrich_l, ostrich_opt_l, cobra_pre_l, cobra_l):

    fig = plt.figure(figsize=(12,12))
    plt.plot(ostrich_l[0], map(get_K, ostrich_l[1]), '.-', color ='r', label='OSTRICH-1F',markevery=5)
    plt.plot(ostrich_opt_l[0],  map(get_K, ostrich_opt_l[1]), '>-', color = 'orange', label='OSTRICH-2F',markevery=5)
    plt.plot(cobra_pre_l[0],  map(get_K, cobra_pre_l[1]), 'x-', color = 'royalblue', label='COBRA-PRE_FIX_UP',markevery=5)
    plt.plot(cobra_pre_l[0],  map(get_K, cobra_l[1]), '^-', color ='b', label='COBRA-OUT_OF_ORDER',markevery=5)
    plt.ylabel('Processed Triples (K)')
    plt.xlabel('Version')
    art = []
    lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
    art.append(lgd)
    plt.savefig("added_plot.png", additional_artists=art, bbox_inches="tight")


# def make_line_plot_ingestion_time_cobra(cobra_l):
#
#     fig = plt.figure()
#     ax = fig.add_subplot(111)
#     ax.plot(np.arange(len(cobra_l[0])), map(get_min, cobra_l[2]), 'xb-', label='COBRA')
#     ax.set_ylabel('Duration (min)')
#     ax.set_xlabel('Version')
#     ax.set_xticks(range(len(cobra_l[3])))
#     ax.set_xticklabels(cobra_l[0])
#     art = []
#     lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
#     art.append(lgd)
#     fig.savefig("cobra_ingestion_plot.png", additional_artists=art, bbox_inches="tight")
#
# def make_line_plot_storage_size_cobra(cobra_l):
#
#     fig = plt.figure()
#     ax = fig.add_subplot(111)
#     ax.plot(map(get_MB, cobra_l[3]), 'xb-', label='COBRA')
#     ax.set_ylabel('Size (MB)')
#     ax.set_xlabel('Version')
#     ax.set_xticks(range(len(cobra_l[3][::5])))
#     ax.set_xticklabels(cobra_l[0])
#     art = []
#     lgd = plt.legend(loc=9, bbox_to_anchor=(0.40, -0.2), ncol=2)
#     art.append(lgd)
#     fig.savefig("cobra_storage_plot.png", additional_artists=art, bbox_inches="tight")

def make_table(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, post_fix_up_cobra_tup, cobra_tup):
    ostrich_dur = str(get_min(ostrich_tup[2][-1]))
    ostrich_mod_dur = str(get_min(ostrich_mod_tup[2][-1]))
    pre_fix_up_cobra_dur = str(get_min(pre_fix_up_cobra_tup[2][-1]))
    post_fix_up_cobra_dur = str(get_min(post_fix_up_cobra_tup[2][-1]))
    cobra_dur = str(get_min(cobra_tup[2][-1]))

    ostrich_size = str(get_MB(ostrich_tup[3][-1]))
    ostrich_mod_size = str(get_MB(ostrich_mod_tup[3][-1]))
    pre_fix_up_cobra_size = str(get_MB(pre_fix_up_cobra_tup[3][-1]))
    post_fix_up_cobra_size = str(get_MB(post_fix_up_cobra_tup[3][-1]))
    cobra_size =  str(get_MB(cobra_tup[3][-1]))

    print("Storage Layout & Storage Size (MB) & Ingestion Time (min) \\ \hline")
    print("OSTRICH & " + ostrich_size + " & " + ostrich_dur +"\\\\")
    print("OSTRICH mod & " + ostrich_mod_size + " & " + ostrich_mod_dur + "\\\\")
    print("COBRA pre fix-up & " + pre_fix_up_cobra_size + " & " + pre_fix_up_cobra_dur + "\\\\")
    print("COBRA post fix-up & " + post_fix_up_cobra_size + " & " + pre_fix_up_cobra_dur + " + " + post_fix_up_cobra_dur + "\\\\")
    print("COBRA & " + cobra_size + " & " + cobra_dur + "\\\\")



# read lines
# MAIN
if(len(sys.argv)!=6):
    print "Not enough arguments: ostrich_path, ostrich_opt_path, pre_fix_up_cobra_path, fix_up_cobra_path, cobra_path"
    sys.exit(-1)

ostrich_path = str(sys.argv[1]);
ostrich_mod_path = str(sys.argv[2]);
pre_fix_up_cobra_path = str(sys.argv[3]);
fix_up_cobra_path = str(sys.argv[4]);
cobra_path = str(sys.argv[5]);

fileA = open(ostrich_path)
fileB = open(ostrich_mod_path)
fileC = open(pre_fix_up_cobra_path)
fileD = open(fix_up_cobra_path)
fileE = open(cobra_path)

ostrich_tup = process_file(fileA)
ostrich_mod_tup = process_file(fileB)
pre_fix_up_cobra_tup = process_pre_fix_up_file(fileC)
post_fix_up_cobra_tup = process_file(fileD)
cobra_tup = process_file(fileE)

make_line_plot_ingestion_time(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, cobra_tup)
make_line_plot_total_ingestion_time(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, cobra_tup)
make_line_plot_storage_size(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, cobra_tup)
make_line_plot_added(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, cobra_tup)
make_table(ostrich_tup, ostrich_mod_tup, pre_fix_up_cobra_tup, post_fix_up_cobra_tup, cobra_tup)
