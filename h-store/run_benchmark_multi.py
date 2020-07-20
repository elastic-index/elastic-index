#!/usr/bin/env python
import os
import sys
import subprocess
import shutil

LEAFSLOTS_ID_STR="LEAFSLOTS_ID_STR"
LEAFSLOTS_FILE='stx-btree-blindi/include/stx/btree.h'
LEAFSLOTS_TARGETS_STRS=["static const int leafslots = {};".format(i) for i in [16,32,64,128]]*2

TREE_LEVEL_ID_STR="TREE_LEVEL_ID_STR"
TREE_LEVEL_FILE="stx-btree-blindi/include/blindi/blindi_seqtree_with_duplicates.hpp"
TREE_LEVEL_TARGET_STRS=["#define TREE_LEVELS {}".format(i) for i in [0,2,2,2]]*2

BLINDI_BREATH_SIZE_ID_STR="BLINDI_BREATH_SIZE_ID_STR"
BLINDI_BREATH_SIZE_FILE="stx-btree-blindi/include/blindi/blindi_seqtree_with_duplicates.hpp"
BLIND_BREATH_SIZE_STRS=["//#define BREATHING_BLINDI_SIZE 8"]*4 + ["#define BREATHING_BLINDI_SIZE 4"]*4

BLINDI_BREATH_DATA_ONLY_ID_STR="BLINDI_BREATH_DATA_ONLY_ID_STR"
BLINDI_BREATH_DATA_ONLY_FILE="stx-btree-blindi/include/blindi/blindi_seqtree_with_duplicates.hpp"
BLINDI_BREATH_DATA_ONLY_STRS=["//#define BLINDI_BREATH_DATA_ONLY"]*4 + ["#define BLINDI_BREATH_DATA_ONLY"]*4

ID_STR_LIST=[LEAFSLOTS_ID_STR,TREE_LEVEL_ID_STR,BLINDI_BREATH_DATA_ONLY_ID_STR]
FILE_LIST=[LEAFSLOTS_FILE,TREE_LEVEL_FILE,BLINDI_BREATH_SIZE_FILE,BLINDI_BREATH_DATA_ONLY_FILE]
STR_LISTS=[LEAFSLOTS_TARGETS_STRS,TREE_LEVEL_TARGET_STRS,BLIND_BREATH_SIZE_STRS,BLINDI_BREATH_DATA_ONLY_STRS]

CONFIG_COPY_SET = {LEAFSLOTS_FILE,
               TREE_LEVEL_FILE,
               BLINDI_BREATH_SIZE_FILE,
               BLINDI_BREATH_DATA_ONLY_FILE}
PER_RUN_COPY_SET={"h-store/index_memory.csv",
                  "h-store/obj/logs/",
                  "h-store/output.out"}

def replace_file_content(id_str_list,file_list,str_list,root_directory):
    for i in range(len(id_str_list)):
        id_str = id_str_list[i]
        file_str = os.path.abspath(root_directory + "/"+file_list[i])
        line_str=str_list[i]
        # obtain the line in which ID id_str exists - notice that grep counts from 1 whereas data is indexed
        # at zero, but since ID_STR appears one line before the line we actually want to replace, no need to change
        # this index
        out = subprocess.check_output("cat {} | grep -n {} | awk -F: \'{{print $1}}\' | head -1  ".format(file_str,id_str),shell=True)
        line_num = int(out)
        # read
        with open(file_str,'r') as f:
            data = f.readlines()
        # replace the line
        data[line_num] = line_str +'\n'
        # write back
        with open(file_str,'w') as f:
            f.writelines(data)




def run_bemchmarks_multi(id_str_list, file_list, str_lists, config_copy_set, per_run_copy_set, benchmark_command, run_per_config, root_directory=".", out_dir="./multi_benchmark_results"):
    # assert (len(id_str_list) == len(file_list)) and (len(file_list) == len(str_lists)), "argument first level lists are of different length!"
    # assert len(set([len(x) for x in str_lists]))==1, "String (second level) lists are of different length!"

    # cd to root directory
    root_directory = os.path.abspath(root_directory)
    out_dir = os.path.abspath(out_dir)
    os.chdir(root_directory)
    # create log dir if not exists
    if not os.path.exists(out_dir):
        os.makedirs(out_dir)

    config_count = len(str_lists[0])
    for i in range(config_count):
        # cd to root directory
        os.chdir(root_directory)
        curr_str_list = [l[i] for l in str_lists]
        replace_file_content(id_str_list,file_list,curr_str_list,root_directory)
        curr_config_our_dir= os.path.abspath(out_dir + "/config_{}".format(i + 1))
        if os.path.exists(curr_config_our_dir):
            print("{} Already Exists!".format(os.path.abspath(curr_config_our_dir)))
            exit(-1)
        os.makedirs(curr_config_our_dir)
        for p in config_copy_set:
            if os.path.isdir(root_directory+"/"+p):
                shutil.copytree(root_directory+"/"+p, curr_config_our_dir + "/" + p)
            else:
                shutil.copy2(root_directory+"/"+p, curr_config_our_dir + "/"+os.path.split(os.path.abspath(root_directory+"/"+p))[1])
        os.chdir("h-store")
        os.system("ant clean")
        #os.system("ant build -Dsite.exec_ee_log_level=INFO")
        os.system("ant build")

        os.system("ant hstore-prepare -Dproject=tpcc -Dhosts=./cluster_config.txt")
        for r in range(run_per_config):
            curr_run_out_dir = os.path.abspath("{}/run_{}".format(curr_config_our_dir,r))
            os.makedirs(curr_run_out_dir)
            os.system(benchmark_command)
            for p in per_run_copy_set:
                if os.path.isdir(root_directory+"/"+p):
                    shutil.copytree(root_directory+"/"+p, curr_run_out_dir + "/" + p)
                else:
                    shutil.copy2(root_directory+"/"+p, curr_run_out_dir + "/"+((os.path.split(os.path.abspath(root_directory+"/"+p)))[1]))



if __name__ == "__main__":
    benchmark_command="ant hstore-benchmark -Dclient.scalefactor=3.0 -Dproject=tpcc  -Dclient.output_index_memory_stats=index_memory.csv -Doutput_results_json=true -Dclient.warmup=60000 -Dclient.duration=360000 -Dclient.txnrate=100000 -Dhosts=./cluster_config.txt -Dclient.threads_per_host=8 -Dclient.jvm_asserts=false  > output.out"
    run_per_config = 2
    run_bemchmarks_multi(id_str_list=ID_STR_LIST,file_list=FILE_LIST,str_lists=STR_LISTS,config_copy_set=CONFIG_COPY_SET,per_run_copy_set=PER_RUN_COPY_SET,benchmark_command=benchmark_command,run_per_config=run_per_config)
