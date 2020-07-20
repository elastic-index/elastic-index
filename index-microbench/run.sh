#!/bin/bash

# export LD_PRELOAD=/usr/local/lib/gcc-5.3.0/lib64/libstdc++.so

RUNS=10
# DEBUG_MODE=1
THREAD_COUNT_LIST="1 2 4 8 16 32 48 64 80"
TREE_LEVEL=2
HYBRID=50

HYBRID_LIST="50 75 90 100"
TREE_LEVEL_LIST="0 2"

case "$1" in
"" ) echo "### The first argument is missing; must be: uniform, zipfian, mono, string or uuid."; exit $E_WRONGARGS;; 
"uniform"|"zipfian"|"mono"|"string"|"uuid" ) KEY_TYPE=$1;;
* ) echo "###  The first argument is invalid; must be: uniform, zipfian, mono, string or uuid."; exit $E_WRONGARGS;;
esac

# case "$KEY_TYPE" in
# "rand"|"mono" ) TARGET="workload";;
# "string"|"uuid" ) TARGET="workload";;#TARGET="workload_string";;
# esac

case "$KEY_TYPE" in
"string" ) MAKE_ARGS="STRING_KEY=1";;
"uuid" )   MAKE_ARGS="UUID_KEY=1";;
esac

run() {
  if [ -z "$DEBUG_MODE" ]
  then
    printf '\n\033[01;32m'
    echo "### STARTED RUNNING: " $@
    printf '\033[00m'
    L_BEGIN_TIME=`date +%s`
    $@
    L_END_TIME=`date +%s`
    L_RUNTIME=$((L_END_TIME-L_BEGIN_TIME))
    printf '\033[01;32m'
    echo "### FINISHED RUNNING: " $@
    printf '### RUNTIME: %dh:%dm:%ds\n' $(($L_RUNTIME/3600)) $(($L_RUNTIME%3600/60)) $(($L_RUNTIME%60))
    printf '\033[00m\n\n'
  else
    echo "### EXEC: " $@
    read answer
  fi
  return 0
}

remake() {
  echo "### Called remake:"
  # echo "###   * make TREE_LEVELS=$TREE_LEVEL $MAKE_ARGS $TARGET"
  # echo "### Called remake:"
  run make clean
  if [ -z "$TREE_LEVEL" ] && [ -z "$HYBRID" ];
  then
    echo "\n\n\n\nUNREACHABLE CODE ALERT"
    exit 1
  fi
  run make -j16 TREE_LEVELS=$TREE_LEVEL HYBRID_THRESHOLD=$HYBRID $MAKE_ARGS workload
  return 0
}

premake_all() {
  echo "### Called premake_all:"
  run make clean

  for TL in $TREE_LEVEL_LIST; do
    for H in $HYBRID_LIST; do
      run make clean
      run make -j16 TREE_LEVELS=$TL HYBRID_THRESHOLD=$H $MAKE_ARGS workload
      TARGET="workload-tl$TL-h$H"
      run mv workload $TARGET
    done
  done
  
  return 0
}

do_runs() {
  INDEX_TYPE=$1
  INNER_SLOTS=16
  LEAF_SLOTS=$2
  TL=$3
  HBRD=$4

  if [ -z "$TL" ]
  then
    if [ -z "$LEAF_SLOTS" ]
    then
      INDEX_NAME="${INDEX_TYPE}"
    else
      INDEX_NAME="${INDEX_TYPE}-${LEAF_SLOTS}"
    fi
  else
    if [ -z "$LEAF_SLOTS" ]
    then
      INDEX_NAME="${INDEX_TYPE}-lvl${TL}"
    else
      INDEX_NAME="${INDEX_TYPE}-${LEAF_SLOTS}-lvl${TL}"
    fi
  fi
  if [ -z "$LEAF_SLOTS" ]
  then
    LEAF_SLOTS=128
    echo "### No leaf-slot number specified, proceeding with the default (128)"
  fi

  if [ "$INDEX_TYPE" = "stx_hybrid" ]; then
    INDEX_NAME="${INDEX_NAME}-hybrid${HBRD}"
  fi

  echo "### Doing runs of $1 with $2 leaf slots at tree level $3"

  if [ -z "$TL" ]; then
    TL=0
  fi
  if [ -z "$HBRD" ]; then
    HBRD=100
  fi
  TARGET="workload-tl${TL}-h${HBRD}"
  # if [ "$INDEX_TYPE" = "stx_hybrid" ]; then
  #   TARGET="workload-tl${TL}-h${HBRD}"
  # fi

  CMD="./$TARGET $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT --inner-slots=$INNER_SLOTS --leaf-slots=${LEAF_SLOTS}"
  OUTPUT="results/${INDEX_NAME}_${WORKLOAD_TYPE}_${KEY_TYPE}_th${THREAD_COUNT}_run${RUN}"
  # echo
  # echo "=== RUN:    #$RUN"
  # echo "=== CMD:    $CMD"
  # echo "=== OUTPUT: $OUTPUT"
  # echo
  # echo ===========================================
  # echo RUN=$RUN  CMD=$CMD
  # echo ===========================================
  if [ -e "$OUTPUT" ]; then
    echo skipping
    continue
  fi
  run $CMD 2>&1 | tee ${OUTPUT}.tmp
  if [ $? -ne 0 ]; then
    echo exiting; status=$?
    exit 1
  fi
  run mv ${OUTPUT}.tmp ${OUTPUT}
  echo
  COUNTER=$(($COUNTER+1))
  return 0
}

run_single() {
  for RUN in `seq 1 $RUNS`; do
    TREE_LEVEL=2
    THREAD_COUNT=1

    for HYBRID in HYBRID_LIST; do
      # remake
      for WORKLOAD_TYPE in a c e f; do
        do_runs stx_hybrid 16 2
      done
    done

    TREE_LEVEL=2
    THREAD_COUNT=1
    # remake
    # RUNS=10
    for WORKLOAD_TYPE in a c e f; do
      do_runs hot
      do_runs stx 16
      do_runs stx_seqtree 128 2
    done
    for WORKLOAD_TYPE in a c e f; do
      do_runs stx_seqtree 16 2
    done
  done
  return 0
}

run_multi() {
  for RUN in `seq 1 $RUNS`; do
    TREE_LEVEL=2
    for THREAD_COUNT in $THREAD_COUNT_LIST; do
      for WORKLOAD_TYPE in a c f; do
        do_runs concur_hot
        do_runs btreeolc 16
        do_runs btreeolc_seqtree 32 2
        do_runs btreeolc_seqtree 64 2
        do_runs btreeolc_seqtree 128 2
      done
    done
    TREE_LEVEL=0
    for THREAD_COUNT in $THREAD_COUNT_LIST; do
      for WORKLOAD_TYPE in a c f; do
        do_runs btreeolc_seqtree 16 0
      done
    done
  done
  return 0
}

# run_test() {
#   premake_all
#   TREE_LEVEL=2
#   THREAD_COUNT=1
#   WORKLOAD_TYPE=f
#   # HYBRID=100
#   for RUN in `seq 1 $RUNS`; do
#     do_runs stx_hybrid 16 2 100
#     do_runs hot
#     do_runs concur_hot
#     do_runs stx 16
#   done
#   return 0
# }

main() {
  premake_all
  for TASK in run_single run_multi; do # run_single run_multi; do
    printf '\n\n\033[01;32mSTARTING A TASK: %s\033[00m\n' $TASK
    START_TIME=`date +%s`
    COUNTER=0
    $TASK
    END_TIME=`date +%s`
    RUNTIME=$((END_TIME-START_TIME))
    printf '\n\n\033[01;32mTotal runtime: %dh:%dm:%ds\n' $(($RUNTIME/3600)) $(($RUNTIME%3600/60)) $(($RUNTIME%60))
    printf 'Total benchmark runs: %d\n' $COUNTER
    printf '\n\n\033[01;32mENDING A TASK: %s\033[00m\n\n' $TASK
  done
  return 0
}


# run ./workload
main
exit 0
