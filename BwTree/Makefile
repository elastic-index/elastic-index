
CXX = g++-5.3.0 -std=c++14
# CXX = g++-5 -std=c++14
PAPI_FLAG = -I../papi/include -L../papi/lib -lpapi
# PAPI_FLAG = -lpapi
CXX_FLAG = -I.. -I../blindi_seqtree/ -I../blindi_subtrie/ -Ibenchmark/hot -pthread -std=c++14 -g -Wall -mcx16 -Wno-invalid-offsetof $(PAPI_FLAG)
GMON_FLAG =


TREE_LEVELS=2
HYBRID_THRESHOLD=100
PP_FLAG = -DTREE_LEVELS=$(TREE_LEVELS) -DSHOULD_COMPRESS_PERCENT=$(HYBRID_THRESHOLD) -DNDEBUG -DUSE_HOT -DBWTREE_NODEBUG

OPT_FLAG = -O2 -Ofast -frename-registers -march=native $(PP_FLAG)
PRELOAD_LIB = LD_PRELOAD=/usr/lib/x86_64-linux-gnu/libjemalloc.so
SRC = ./test/benchmark_full.h ./test/main.cpp test/memusage.cpp ./src/bwtree.h ./src/bloom_filter.h ./src/atomic_stack.h ./src/sorted_small_set.h ./test/test_suite.h ./test/test_suite.cpp ./benchmark/spinlock/spinlock.cpp ./benchmark/BTreeOLC/BTreeOLC.hpp ./benchmark/BTreeOLC/blindi_BTreeOLC_seqtree.hpp ./benchmark/skip_list/skip_list_lazy.hpp ./benchmark/skip_list/skip_list_single_lock.hpp ./benchmark/skip_list/skip_list_seq.hpp ./benchmark/skip_list/skip_list_lock_free.hpp
OBJ = ./build/memusage.o ./build/bwtree.o ./build/test_suite.o ./build/spinlock.o ./build/art.o

#leafs=  16 32 64 128 256 512 32000
#inner=  16 32 64 128 256 512 1024
leafs=  16 32 64 128 256 512 
inner=  16 

t0=     $(addprefix -inner=, $(inner))
t1=     $(foreach i, $(t0), $(addprefix $(i)-leafs=, $(leafs)))
targets=        $(addprefix main, $(t1))

all:    $(targets)

$(targets): $(OBJ) ./test/main.cpp
	@echo Compiling $@; \
	leaf=`echo $@ | awk -F- '{print $$3}' | awk -F= '{print $$2}'`; \
	inner=`echo $@ | awk -F- '{print $$2}' | awk -F= '{print $$2}'`; \
	$(CXX) ./test/main.cpp $(OBJ) -o ./main-inner=$${inner}-leaf=$${leaf} -DLEAF_NUM=$${leaf} -DINNER_NUM=$${inner} $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG); \

./build/memusage.o: $(SRC) ./src/bwtree.h
	$(CXX) ./test/memusage.cpp -c -o ./build/memusage.o $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG)

./build/art.o:
	$(CXX) ./benchmark/art/art.c -c -o ./build/art.o $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG)

./build/bwtree.o: ./src/bwtree.cpp ./src/bwtree.h
	$(CXX) ./src/bwtree.cpp -c -o ./build/bwtree.o $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG)

./build/test_suite.o: ./test/test_suite.cpp ./src/bwtree.h
	$(CXX) ./test/test_suite.cpp -c -o ./build/test_suite.o $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG)

./build/spinlock.o:
	$(CXX) ./benchmark/spinlock/spinlock.cpp -c -o ./build/spinlock.o $(CXX_FLAG) $(OPT_FLAG) $(GMON_FLAG)

gprof:
	make clean
	make all GMON_FLAG=-pg

full-speed:
	make clean
	make OPT_FLAG=" -Ofast -frename-registers -funroll-loops -flto -march=native -DNDEBUG -DBWTREE_NODEBUG -lboost_system -lboost_thread"

small-size:
	make clean
	make OPT_FLAG=" -Os -DNDEBUG -DBWTREE_NODEBUG"

benchmark-bwtree-full: main
	$(PRELOAD_LIB) ./main --benchmark-bwtree-full

benchmark-btree-full: main
	$(PRELOAD_LIB) ./main --benchmark-btree-full

benchmark-blindi-full: main
	$(PRELOAD_LIB) ./main --benchmark-blindi-full

benchmark-art-full: main
	$(PRELOAD_LIB) ./main --benchmark-art-full

prepare:
	mkdir -p build
	mkdir -p ./stl_test/bin

clean:
	rm -f ./build/*
	rm -f *.log
	rm -f ./main
	rm -rf ./main-*
