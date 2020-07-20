#!/bin/bash

RUNS=1
export LD_PRELOAD=/usr/local/lib/gcc-5.3.0/lib64/libstdc++.so


# run ./workload_string
for RUN in `seq 1 $RUNS`; do
  for KEY_TYPE in uuid; do
    for WORKLOAD_TYPE in c a; do
      for THREAD_COUNT in 1 2 20 40; do
        for TL in `seq 0 3`; do
          make clean
          make TREE_LEVELS=$TL UUID_KEY=1 workload_string

          for INDEX_TYPE in stx_seqtree btreeolc_seqtree; do
              
            if [ "$INDEX_TYPE" = "stx_seqtree" ] && [ "$THREAD_COUNT" -ne 1 ]; then
              continue
            fi

            CMD="./workload_string $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT --leaf-slots=16"
            OUTPUT="results/result_${THREAD_COUNT}_${KEY_TYPE}_${WORKLOAD_TYPE}_${INDEX_TYPE}16_TL${TL}_${RUN}"
            echo
            echo ===========================================
            echo RUN=$RUN  CMD=$CMD
            echo ===========================================
            if [ -e "$OUTPUT" ]; then
              echo skipping
              continue
            fi
            $CMD 2>&1 | tee ${OUTPUT}.tmp
            if [ $? -ne 0 ]; then
              echo exiting; status=$?
              exit 1
            fi
            mv ${OUTPUT}.tmp ${OUTPUT}
            echo

            CMD="./workload_string $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT --leaf-slots=128"
            OUTPUT="results/result_${THREAD_COUNT}_${KEY_TYPE}_${WORKLOAD_TYPE}_${INDEX_TYPE}128_TL${TL}_${RUN}"
            echo
            echo ===========================================
            echo RUN=$RUN  CMD=$CMD
            echo ===========================================
            if [ -e "$OUTPUT" ]; then
              echo skipping
              continue
            fi
            $CMD 2>&1 | tee ${OUTPUT}.tmp
            if [ $? -ne 0 ]; then
              echo exiting; status=$?
              exit 1
            fi
            mv ${OUTPUT}.tmp ${OUTPUT}
            echo            
          done
        done

        for INDEX_TYPE in  hot concur_hot; do # artolc skiplist masstree bwtree
          if [ "$INDEX_TYPE" = "artolc" ] && [ "$WORKLOAD_TYPE" == "e" ] && [ "$THREAD_COUNT" -eq 40 ]; then
            continue
          fi
          if [ "$INDEX_TYPE" = "hot" ] && [ "$THREAD_COUNT" -ne 1 ]; then
            continue
          fi           
          CMD="./workload_string $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT"
          OUTPUT="results/result_${THREAD_COUNT}_${KEY_TYPE}_${WORKLOAD_TYPE}_${INDEX_TYPE}_${RUN}"
          echo
          echo ===========================================
          echo RUN=$RUN  CMD=$CMD
          echo ===========================================
          if [ -e "$OUTPUT" ]; then
            echo skipping
            continue
          fi
          $CMD 2>&1 | tee ${OUTPUT}.tmp
          if [ $? -ne 0 ]; then
            echo exiting; status=$?
            exit 1
          fi
          mv ${OUTPUT}.tmp ${OUTPUT}
          echo
        done
        for INDEX_TYPE in stx btreeolc; do # stx_trie btreeolc_trie stx_subtrie btreeolc_subtrie
          if [ "$INDEX_TYPE" = "stx" ] && [ "$THREAD_COUNT" -ne 1 ]; then
            continue
          fi
          if [ "$INDEX_TYPE" = "stx_trie" ] && [ "$THREAD_COUNT" -ne 1 ]; then
            continue
          fi
          if [ "$INDEX_TYPE" = "stx_subtrie" ] && [ "$THREAD_COUNT" -ne 1 ]; then
            continue
          fi   
          CMD="./workload_string $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT --leaf-slots=16"
          OUTPUT="results/result_${THREAD_COUNT}_${KEY_TYPE}_${WORKLOAD_TYPE}_${INDEX_TYPE}16_${RUN}"
          echo
          echo ===========================================
          echo RUN=$RUN  CMD=$CMD
          echo ===========================================
          if [ -e "$OUTPUT" ]; then
            echo skipping
            continue
          fi
          $CMD 2>&1 | tee ${OUTPUT}.tmp
          if [ $? -ne 0 ]; then
            echo exiting; status=$?
            exit 1
          fi
          mv ${OUTPUT}.tmp ${OUTPUT}
          echo  
          CMD="./workload_string $WORKLOAD_TYPE $KEY_TYPE $INDEX_TYPE $THREAD_COUNT --leaf-slots=128"
          OUTPUT="results/result_${THREAD_COUNT}_${KEY_TYPE}_${WORKLOAD_TYPE}_${INDEX_TYPE}128_${RUN}"
          echo
          echo ===========================================
          echo RUN=$RUN  CMD=$CMD
          echo ===========================================
          if [ -e "$OUTPUT" ]; then
            echo skipping
            continue
          fi
          $CMD 2>&1 | tee ${OUTPUT}.tmp
          if [ $? -ne 0 ]; then
            echo exiting; status=$?
            exit 1
          fi
          mv ${OUTPUT}.tmp ${OUTPUT}
          echo
        done
      done
    done
  done
done
