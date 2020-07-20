CC = gcc-5
CXX = g++-5 -std=c++14

TREE_LEVELS = 2
HYBRID_THRESHOLD=70
PREPROCESSOR = -DTREE_LEVELS=$(TREE_LEVELS) -DSHOULD_COMPRESS_PERCENT=$(HYBRID_THRESHOLD) -DNDEBUG -DUSE_HOT -DBWTREE_NODEBUG #-DDEBUG_PRINT=1 -DBTREEOLC_DEBUG=1 -DDEBUG_PRINT=1 -DBTREE_DEBUG #-DDEBUG_PRINT

DEPSDIR := masstree/.deps
DEPCFLAGS = -MD -MF $(DEPSDIR)/$*.d -MP
MEMMGR = -lpapi -l:libtcmalloc_minimal.so.4
PAPI_FLAG = -I../papi/include -L../papi/lib -lpapi
CFLAGS = -g -O3 -march=native -Wno-invalid-offsetof -mcx16 $(DEPCFLAGS) $(PAPI_FLAG)
BLINDI_INCLUDES = -I.. -I../blindi_seqtree/ -I../blindi_subtrie/ -I../BwTree/benchmark/BTreeOLC -I../BwTree/benchmark/stx_blindi -I../BwTree/benchmark/stx_btree -I./hot #-I../BwTree/benchmark/hot
BTREEOLC_DIR = ../BwTree/benchmark/BTreeOLC
STX_DIR = ../BwTree/benchmark/stx_btree
STX_BLINDI_DIR = ../BwTree/benchmark/stx_blindi
STX_H  = $(STX_DIR)/btree.h $(STX_DIR)/btree_multimap.h $(STX_BLINDI_DIR)/blindi_btree_trie.h $(STX_BLINDI_DIR)/blindi_btree_subtrie.h $(STX_BLINDI_DIR)/blindi_btree_seqtree.h $(STX_BLINDI_DIR)/blindi_btree_multimap_trie.h $(STX_BLINDI_DIR)/blindi_btree_multimap_subtrie.h $(STX_BLINDI_DIR)/blindi_btree_multimap_seqtree.h $(STX_BLINDI_DIR)/blindi_btree_hybrid_nodes.h $(STX_BLINDI_DIR)/blindi_btree_hybrid_nodes_multimap.h
BTREEOLC_H = $(BTREEOLC_DIR)/blindi_BTreeOLC_seqtree.hpp $(BTREEOLC_DIR)/blindi_BTreeOLC_subtrie.hpp $(BTREEOLC_DIR)/blindi_BTreeOLC_trie.hpp
INCLUDES = -include masstree/config.h  -I.. $(BLINDI_INCLUDES)
CFLAGS += $(INCLUDES) $(PREPROCESSOR)

# # By default just use 1 thread. Override this option to allow running the
# # benchmark with 20 threads. i.e. THREAD_NUM=20 make run_all_atrloc
# THREAD_NUM?=1
# TYPE?=bwtree

# Can also change it to rotate skiplist
SL_DIR=./nohotspot-skiplist

# skiplist source files
SL_OBJS=$(patsubst %.cpp,%.o,$(wildcard $(SL_DIR)/*.cpp))
$(info skip list object files: $(SL_OBJS))

SNAPPY = /usr/lib/libsnappy.so.1.3.0

all: workload

ifdef TREE_LEVELS
$(info ### Note: set seqtree's TREE_LEVEL parameter to ${TREE_LEVELS})
endif
ifdef HYBRID_THRESHOLD
$(info ### Note: set stx_hybrid's compression threshold to ${HYBRID_THRESHOLD})
endif
ifdef STRING_KEY
$(info ### Note: set string as the key type)
CFLAGS += -DUSE_STRING_KEY -DIS_STRING
endif
ifdef UUID_KEY
$(info ### Note: set UUID as the key type)
CFLAGS += -DUSE_UUID_KEY -DIS_STRING
endif

# run_all: workload workload_string
# 	./workload a rand $(TYPE) $(THREAD_NUM)
# 	./workload c rand $(TYPE) $(THREAD_NUM)
# 	./workload e rand $(TYPE) $(THREAD_NUM)
# 	./workload a mono $(TYPE) $(THREAD_NUM)
# 	./workload c mono $(TYPE) $(THREAD_NUM)
# 	./workload e mono $(TYPE) $(THREAD_NUM)
# 	./workload_string a email $(TYPE) $(THREAD_NUM)
# 	./workload_string c email $(TYPE) $(THREAD_NUM)
# 	./workload_string e email $(TYPE) $(THREAD_NUM)

workload.o: $(BTREEOLC_H) $(STX_H) workload.cpp microbench.h index.h util.h ./masstree/mtIndexAPI.hh ./BwTree/bwtree.h BTreeOLC/BTreeOLC.h BTreeOLC/BTreeOLC_child_layout.h ./pcm/pcm-memory.cpp ./pcm/pcm-numa.cpp ./papi_util.cpp
	$(CXX) $(CFLAGS) -c -o workload.o workload.cpp

workload: skiplist-clean workload.o bwtree.o artolc.o btree.o ./masstree/mtIndexAPI.a ./pcm/libPCM.a $(SL_OBJS)
	$(CXX) $(CFLAGS) -o workload workload.o bwtree.o artolc.o btree.o $(SL_OBJS) masstree/mtIndexAPI.a ./pcm/libPCM.a $(MEMMGR) -lpthread -lm -ltbb

# workload_string.o: $(BTREEOLC_H) $(STX_H) workload_string.cpp microbench.h index.h util.h ./masstree/mtIndexAPI.hh ./BwTree/bwtree.h BTreeOLC/BTreeOLC.h skiplist-clean
# 	$(CXX) $(CFLAGS) -DIS_STRING  -c -o workload_string.o workload_string.cpp

# workload_string.o:  $(BTREEOLC_H) $(STX_H) workload.cpp microbench.h index.h util.h ./masstree/mtIndexAPI.hh ./BwTree/bwtree.h BTreeOLC/BTreeOLC.h BTreeOLC/BTreeOLC_child_layout.h ./pcm/pcm-memory.cpp ./pcm/pcm-numa.cpp ./papi_util.cpp
# 	$(CXX) $(CFLAGS) -c -o workload_string.o workload.cpp

# workload_string: skiplist-clean workload_string.o bwtree.o artolc.o btree.o ./masstree/mtIndexAPI.a ./pcm/libPCM.a $(SL_OBJS)
# 	$(CXX) $(CFLAGS) -o workload_string workload_string.o bwtree.o artolc.o  $(SL_OBJS) masstree/mtIndexAPI.a $(MEMMGR) -lpthread -lm -ltbb

# workload_string.o: $(BTREEOLC_H) $(STX_H) workload_string.cpp microbench.h index.h util.h ./masstree/mtIndexAPI.hh ./BwTree/bwtree.h BTreeOLC/BTreeOLC.h skiplist-clean
# 	$(CXX) $(CFLAGS) -DIS_STRING  -c -o workload_string.o workload_string.cpp

# workload_string: skiplist-clean workload_string.o bwtree.o artolc.o ./masstree/mtIndexAPI.a $(SL_OBJS)
# 	$(CXX) $(CFLAGS) -DIS_STRING  -o workload_string workload_string.o bwtree.o artolc.o  $(SL_OBJS) masstree/mtIndexAPI.a $(MEMMGR) -lpthread -lm -ltbb

bwtree.o: ./BwTree/bwtree.h ./BwTree/bwtree.cpp
	$(CXX) $(CFLAGS) -c -o bwtree.o ./BwTree/bwtree.cpp

artolc.o: ./ARTOLC/*.cpp ./ARTOLC/*.h
	$(CXX) $(CFLAGS) ./ARTOLC/Tree.cpp -c -o artolc.o $(MEMMGR) -lpthread -lm -ltbb

btree.o: ./btree-rtm/*.c ./btree-rtm/*.h
	$(CXX) $(CFLAGS) ./btree-rtm/btree.c -c -o btree.o $(MEMMGR) -lpthread -lm

$(SL_DIR)/%.o: $(SL_DIR)/%.cpp $(SL_DIR)/%.h
	$(CXX) $(CFLAGS) -c -o $@ $< $(MEMMGR) -lpthread -lm -ltbb

# generate_workload:
# 	./generate_all_workloads.sh

clean:
	$(RM) workload workload_string *.o *~ *.d
	$(RM) $(SL_DIR)/*.o

skiplist-clean:
	$(RM) rm $(SL_DIR)/*.o
