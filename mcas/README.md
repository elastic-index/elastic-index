We have integrated the Elastic index into MCAS as an ADO component.

1. Compile MCAS:  (See mcas/README.md)
- Run dependencies for your OS:
           `cd deps; ./install-<Your-OS-Version>.sh; cd ../`
- Cmake:
           `cd  build/ ; cmake -DBUILD_KERNEL_SUPPORT=ON -DBUILD_PYTHON_SUPPORT=ON -DBUILD_MPI_APPS=0 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX:PATH=`pwd`/dist ..`
- Bootstrap: `make bootstrap`
- Build: `make -j install`

2. The trace file should be located at `mcas/examples/personalities/cpp_rangeindex/data/trace.txt`

   This repo already contains this file, but it consists of only 1000 rows (due to space issues). The full 48 M-row file used for the paper is available at https://drive.google.com/file/d/1sX7XExfo4mu9liGecWwU1uMMiJZp04NW/view?usp=sharing (compressed size: 304 MB, uncompressed size: 2.3 GB).

3. Our evaluation firsts insert all the rows from the trace file to a table that resides in the ADO. In every insert, we add an item to the elastic index.  After the insert finishes, we perform 3 M random GetValue calls.

   The Elastic index library is in `mcas/src/lib/stx_blindi`.

- To change the Elastic parameters you can comment line 62 and remove the comment from any line in the range of  60 to 65 in the file `mcas/src/lib/stx_blindi/blindi_btree_hybrid_nodes.h`.
- The default in this example is 2400000 (line 62), which means it will start to compress the STX leaves to the compact representation after 24 M items are inserted (which is 50% of 48 M items).

   To run the test:
   Server:
       `./dist/bin/mcas --conf ./dist/conf/cpp-rangeindex.conf`

   Client:
       `./dist/bin/personality-cpp-rangeindex-test --data ../examples/personalities/cpp_rangeindex/data/trace.txt --server <IP address>`

