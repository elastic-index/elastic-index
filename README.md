
# Elastic indexes code

## Operation benchmarks (paper: Sections 6.1 & 6.4)

This benchmark is in the `BwTree` directory:

    cd BwTree
    mkdir build
    make

This produces several binaries for various Blindi variants and parameters.

SeqTree:

    # inner_node size=16, leaf_node=128, TREE_LEVEL=2, BREATHING=4
    ./main-inner=16-leaf=128  --index=blindi_seqtree --key=long-rand --key_num=20000000

B+tree:

    # inner_node size=16, leaf_node=16
    ./main-inner=16-leaf=16  --index=btree --key=long-rand --key_num=20000000

HOT:

    ./main-inner=16-leaf=16  --index=hot --key=long-rand --key_num=20000000

Elastic B+tree:

    # Start compress on 50000000d items
    # The tree is started with inner_node size=16, leaf_node=16 
    ./main-inner=16-leaf=16  --index=blindi_btree_hybrid_nodes --key=long-rand --key_num=60000000

To benchmark range query, change the defines:

    #define GET_RANGE
    #define RANGE_SIZE 10 


## YCSB benchmarks (paper: Section 6.2)

This benchmark is in the `index-microbench` directory:

```sh
cd index-microbench
cd masstree
make
``` 

### Build the benchmark application ###

1. Edit Makefile

2. Build masstree

```sh
cd masstree
make
cd ..
``` 

3. Build PCM

```sh
cd pcm
make
cd ..
``` 

4. Build index-microbench


```sh
make
``` 

### Generate YCSB Workloads ###

1. Download [YCSB](https://github.com/brianfrankcooper/YCSB/releases/latest)

   ```sh
   curl -O --location https://github.com/brianfrankcooper/YCSB/releases/download/0.11.0/ycsb-0.11.0.tar.gz
   tar xfvz ycsb-0.11.0.tar.gz
   mv ycsb-0.11.0 YCSB
   ``` 

2. Create Workload Spec 
 
   The default workload a-f are in ./workload_spec 
 
   You can of course generate your own spec and put it in this folder. 

3. Generate

   The script generate_all_workloads.sh specifies which workloads (a--f) to generate for each key (mono, zipfian, uniform, uuid, string). It can be straightforwardly edited to remove unnecessary workloads.

   ```sh
   mkdir workloads
   sh generate_all_workloads.sh
   ```

   The generated workload files will be in ./workloads 

### Run ###

```sh
mkdir results
sh run.sh mono
sh run.sh zipfian
sh run.sh uniform
sh run.sh string
sh run.sh uuid
``` 

## MCAS experiments (paper: Section 6.3)

The code is in the `mcas` directory. See the README file there for instructions on how to compile the system and run the experiments.

