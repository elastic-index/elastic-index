#!/bin/bash

#for KEY_TYPE in monoint randint uuid string; do
for KEY_TYPE in uniform zipfian mono uuid string; do
  for WORKLOAD_TYPE in a c e f; do # e f; do
    echo workload${WORKLOAD_TYPE} > workload_config.inp
    echo ${KEY_TYPE} >> workload_config.inp
    python gen_workload.py workload_config.inp
    mv workloads/load_${KEY_TYPE}_workload${WORKLOAD_TYPE} workloads/${KEY_TYPE}_workload${WORKLOAD_TYPE}_input_50M.dat
    mv workloads/txn_${KEY_TYPE}_workload${WORKLOAD_TYPE}  workloads/${KEY_TYPE}_workload${WORKLOAD_TYPE}_txns_100M.dat
  done
done