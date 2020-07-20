#!/usr/bin/python

import sys
import os

RUNS = 10
MIN_RANGE = 0
MAX_RANGE = 2

def process_a_file(inp_name):
	ins_tput = "ERROR"
	txn_tput = "ERROR"
	ins_mem = "ERROR"
	txn_mem = "ERROR"
	try:
		fin = open(inp_name, 'r')
	except IOError:
		# print "Input file not found: " +  inp_name
		# return (ins_tput, ins_mem, txn_tput, txn_mem)
		return None
	for line in fin:
		words = line.split()
		if "* insert tput" in line:
			ins_tput = words[-1]
		elif "* txns tput" in line:
			txn_tput = words[-1]
		elif "* insert mem" in line:
			ins_mem = words[-1]
		elif "* txns mem" in line:
			txn_mem = words[-1]
	fin.close()
	return (ins_tput, ins_mem, txn_tput, txn_mem)

def make_a_table(wl_type, key_type, th_count, prefix):
	table_name = "{WORKLOAD_TYPE}-{KEY_TYPE}-{THREAD_COUNT}".format(
		**{'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type
	})

	if os.path.exists('parsed/'+prefix) is False:
		try:
			os.mkdir('parsed/'+prefix)
		except OSError:
			print 

	# prefix = "results/"
	input_tmp = prefix + "/{INDEX_NAME}_{WORKLOAD_TYPE}_{KEY_TYPE}_th{THREAD_COUNT}_run{RUN}"
	# input_tmp = "results/result_{THREAD_COUNT}_{KEY_TYPE}_{WORKLOAD_TYPE}_{INDEX_NAME}_{RUN}"
	idx_hybrid_tmp = "{INDEX_TYPE}-{LEAF_SLOTS}-lvl{TL}-hybrid{HYBRID}" #"{INDEX_TYPE}{LEAF_SLOTS}_TL{TL}" #stx_seqtree-16-lvl2
	idx_blindi_tmp = "{INDEX_TYPE}-{LEAF_SLOTS}-lvl{TL}" #"{INDEX_TYPE}{LEAF_SLOTS}_TL{TL}" #stx_seqtree-16-lvl2
	idx_btree_tmp = "{INDEX_TYPE}-{LEAF_SLOTS}" #"{INDEX_TYPE}{LEAF_SLOTS}"
	idx_tmp =  "{INDEX_TYPE}"	
	# suffix_tmp = "_{WORKLOAD_TYPE}_{KEY_TYPE}_th{THREAD_COUNT}_run{RUN}"
	# stx_hybrid-16-lvl2-hybrid30_c_rand_th1_run8

	big_success = False
	out_name = ('parsed/' + prefix + '/{}.txt').format(table_name)
	for leaf_slots in ["16", "32", "64", "128"]:
		for idx_type in ["stx_seqtree", "btreeolc_seqtree"]:
			for lvl in range(11):
				success = False
				idx_name = idx_blindi_tmp.format(**{'INDEX_TYPE': idx_type, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl)})
				(l_ins_tput, l_ins_mem, l_txn_tput, l_txn_mem) = ("", "", "", "")
				for run in range(1,RUNS+1):
					inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
					one_run = process_a_file(inp_name)
					if one_run == None:
						continue
					success = True
					l_ins_tput = l_ins_tput + "\t" + one_run[0]
					l_ins_mem = l_ins_mem + "\t" + one_run[1]
					l_txn_tput = l_txn_tput + "\t" + one_run[2]
					l_txn_mem = l_txn_mem + "\t" + one_run[3]
				if success:
					if not big_success:
						big_success = True
						fout = open(out_name, 'w')
					print out_name + ": added " + idx_name
					fout.write(idx_name + l_ins_tput + l_ins_mem + l_txn_tput + l_txn_mem + "\n")

		for idx_type in ["stx_hybrid"]:
			for lvl in range(11):
				for hybrid in range(0, 101, 5):
					success = False
					idx_name = idx_hybrid_tmp.format(**{'INDEX_TYPE': idx_type, 'HYBRID': hybrid, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl)})
					(l_ins_tput, l_ins_mem, l_txn_tput, l_txn_mem) = ("", "", "", "")
					for run in range(1,RUNS+1):
						inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
						one_run = process_a_file(inp_name)
						if one_run == None:
							continue
						success = True
						l_ins_tput = l_ins_tput + "\t" + one_run[0]
						l_ins_mem = l_ins_mem + "\t" + one_run[1]
						l_txn_tput = l_txn_tput + "\t" + one_run[2]
						l_txn_mem = l_txn_mem + "\t" + one_run[3]
					if success:
						if not big_success:
							big_success = True
							fout = open(out_name, 'w')
						print out_name + ": added " + idx_name
						fout.write(idx_name + l_ins_tput + l_ins_mem + l_txn_tput + l_txn_mem + "\n")

		for idx_type in ["stx", "btreeolc"]:
			success = False
			idx_name = idx_btree_tmp.format(**{'INDEX_TYPE': idx_type, 'LEAF_SLOTS':str(leaf_slots)})
			(l_ins_tput, l_ins_mem, l_txn_tput, l_txn_mem) = ("", "", "", "")
			for run in range(1,RUNS+1):
				inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
				one_run = process_a_file(inp_name)
				if one_run == None:
					continue
				success = True				
				l_ins_tput = l_ins_tput + "\t" + one_run[0]
				l_ins_mem = l_ins_mem + "\t" + one_run[1]
				l_txn_tput = l_txn_tput + "\t" + one_run[2]
				l_txn_mem = l_txn_mem + "\t" + one_run[3]
			if success:
				if not big_success:
					big_success = True
					fout = open(out_name, 'w')
				print out_name + ": added " + idx_name
				fout.write(idx_name + l_ins_tput + l_ins_mem  + l_txn_tput + l_txn_mem + "\n")
	for idx_type in ["hot", "concur_hot"]:
		success = False
		idx_name = idx_tmp.format(**{'INDEX_TYPE': idx_type})
		(l_ins_tput, l_ins_mem, l_txn_tput, l_txn_mem) = ("", "", "", "")
		for run in range(1,RUNS+1):
			inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
			one_run = process_a_file(inp_name)
			if one_run == None:
				continue
			success = True			
			l_ins_tput = l_ins_tput + "\t" + one_run[0]
			l_ins_mem = l_ins_mem + "\t" + one_run[1]
			l_txn_tput = l_txn_tput + "\t" + one_run[2]
			l_txn_mem = l_txn_mem + "\t" + one_run[3]
		if success:
			if not big_success:
				big_success = True
				fout = open(out_name, 'w')			
			print out_name + ": added " + idx_name
			fout.write(idx_name + l_ins_tput  + l_ins_mem  + l_txn_tput  + l_txn_mem + "\n")
	# for leaf_slots in ["16", "32", "64", "128"]:
	# 	for idx_type in ["stx_seqtree", "btreeolc_seqtree"]:
	# 		if th_count > 1 and idx_type == "stx_seqtree":
	# 			continue
	# 		if wl_type == "e" and idx_type == "btreeolc_seqtree":
	# 			continue
	# 		for lvl in range(MIN_RANGE, MAX_RANGE+1):				
	# 			idx_name = idx_blindi_tmp.format(**{'INDEX_TYPE': idx_type, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl)})
	# 			l_ins_tput = ""
	# 			l_ins_mem = ""
	# 			l_txn_tput = ""
	# 			l_txn_mem = ""
	# 			for run in range(1,RUNS+1):
	# 				inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
	# 				one_run = process_a_file(inp_name)
	# 				l_ins_tput = l_ins_tput + "\t" + one_run[0]
	# 				l_ins_mem = l_ins_mem + "\t" + one_run[1]
	# 				l_txn_tput = l_txn_tput + "\t" + one_run[2]
	# 				l_txn_mem = l_txn_mem + "\t" + one_run[3]
	# 			fout.write(idx_name + l_ins_tput + l_ins_mem + l_txn_tput + l_txn_mem + "\n")
	# 	for idx_type in ["stx", "btreeolc"]:
	# 		idx_name = idx_btree_tmp.format(**{'INDEX_TYPE': idx_type, 'LEAF_SLOTS':str(leaf_slots)})
	# 		l_ins_tput = ""
	# 		l_ins_mem = ""
	# 		l_txn_tput = ""
	# 		l_txn_mem = ""
	# 		for run in range(1,RUNS+1):
	# 			inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
	# 			one_run = process_a_file(inp_name)
	# 			l_ins_tput = l_ins_tput + "\t" + one_run[0]
	# 			l_ins_mem = l_ins_mem + "\t" + one_run[1]
	# 			l_txn_tput = l_txn_tput + "\t" + one_run[2]
	# 			l_txn_mem = l_txn_mem + "\t" + one_run[3]
	# 		fout.write(idx_name + l_ins_tput + l_ins_mem  + l_txn_tput + l_txn_mem + "\n")
	# for idx_type in ["hot", "concur_hot"]:
	# 	idx_name = idx_tmp.format(**{'INDEX_TYPE': idx_type})
	# 	l_ins_tput = ""
	# 	l_ins_mem = ""
	# 	l_txn_tput = ""
	# 	l_txn_mem = ""
	# 	for run in range(1,RUNS+1):
	# 		inp_name = input_tmp.format(**{'INDEX_NAME': idx_name, 'LEAF_SLOTS':str(leaf_slots), 'TL': str(lvl), 'THREAD_COUNT' : str(th_count), 'KEY_TYPE' : key_type, 'WORKLOAD_TYPE' : wl_type, 'RUN': str(run)})
	# 		one_run = process_a_file(inp_name)
	# 		l_ins_tput = l_ins_tput + "\t" + one_run[0]
	# 		l_ins_mem = l_ins_mem + "\t" + one_run[1]
	# 		l_txn_tput = l_txn_tput + "\t" + one_run[2]
	# 		l_txn_mem = l_txn_mem + "\t" + one_run[3]
	# 	fout.write(idx_name + l_ins_tput  + l_ins_mem  + l_txn_tput  + l_txn_mem + "\n")
	if big_success:
		fout.close()
	return

if __name__ == "__main__":
	if len(sys.argv) <= 1:
		print "Usage: parse_results.py [name of the benchmark run]"
		sys.exit(2)
	prefix = sys.argv[1]
	for key_type in ["rand"]: #["uuid", "rand", "string"]:
		for wl_type in ["a", "c", "e", "f"]: #["c", "a", "e", "b", "d", "f"]:
			for th_count in range(101):
				make_a_table(wl_type, key_type, th_count, prefix)
