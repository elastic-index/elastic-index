import sys
import os
import uuid
import string
import random

class bcolors:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

#####################################################################################

def reverseHostName ( email ) :
    name, sep, host = email.partition('@')
    hostparts = host[:-1].split('.')
    r_host = ''
    for part in hostparts :
        r_host = part + '.' + r_host
    return r_host + sep + name

def random_string(length):
    allchar = string.ascii_letters + string.punctuation + string.digits
    result = "".join(random.choice(allchar) for x in range(length))
    return result

#####################################################################################

if (len(sys.argv) != 2) :
    print bcolors.WARNING + 'Usage:'
    print 'workload file' + bcolors.ENDC

config_file = sys.argv[1]

args = []
f_config = open (config_file, 'r')
for line in f_config :
    args.append(line[:-1])

ycsb_dir = 'YCSB/'
workload_dir = 'workload_spec/'
output_dir='workloads/'

workload = args[0]
key_type = args[1]

workload_type = ""
if key_type == 'mono':
    workload_type = 'mono_' + workload[8:]
elif key_type == 'zipfian':
    workload_type = 'zipfian_' + workload[8:]
elif key_type == 'uniform':
    workload_type = 'uniform_' + workload[8:]
elif key_type == 'string':
    workload_type = 'string_' + workload[8:]
elif key_type == 'uuid':
    workload_type = 'uuid_' + workload[8:]
else:
    print "??????"

print
print bcolors.OKGREEN + '##########' + bcolors.ENDC
print bcolors.OKGREEN + 'workload = ' + workload_type
print 'key type = ' + key_type + bcolors.ENDC

email_list = 'list.txt'
email_list_size = 27549660

out_ycsb_load = output_dir + 'ycsb_load_' + key_type + '_' + workload
out_ycsb_txn = output_dir + 'ycsb_txn_' + key_type + '_' + workload
out_load_ycsbkey = output_dir + 'load_' + 'ycsbkey' + '_' + workload
out_txn_ycsbkey = output_dir + 'txn_' + 'ycsbkey' + '_' + workload
out_load = output_dir + 'load_' + key_type + '_' + workload
out_txn = output_dir + 'txn_' + key_type + '_' + workload

cmd_ycsb_load = ycsb_dir + 'bin/ycsb load basic -P ' + workload_dir + workload_type + ' -s > ' + out_ycsb_load
cmd_ycsb_txn = ycsb_dir + 'bin/ycsb run basic -P ' + workload_dir + workload_type + ' -s > ' + out_ycsb_txn

print bcolors.OKGREEN + '\nStarting generating inserts\n' + bcolors.ENDC
os.system(cmd_ycsb_load)
print bcolors.OKGREEN + '\nStarting generating transactions\n' + bcolors.ENDC
os.system(cmd_ycsb_txn)
print bcolors.OKGREEN + '\nStarting postprocessing the generated data\n' + bcolors.ENDC

#####################################################################################

f_load = open (out_ycsb_load, 'r')
f_load_out = open (out_load_ycsbkey, 'w')
for line in f_load :
    cols = line.split()
    if len(cols) > 0 and cols[0] == "INSERT":
        f_load_out.write (cols[0] + " " + cols[2][4:] + "\n")
f_load.close()
f_load_out.close()

f_txn = open (out_ycsb_txn, 'r')
f_txn_out = open (out_txn_ycsbkey, 'w')
for line in f_txn :
    cols = line.split()
    if (cols[0] == 'SCAN') or (cols[0] == 'INSERT') or (cols[0] == 'READ') or (cols[0] == 'UPDATE'):
        startkey = cols[2][4:]
        if cols[0] == 'SCAN' :
            numkeys = cols[3]
            f_txn_out.write (cols[0] + ' ' + startkey + ' ' + numkeys + '\n')
        else :
            f_txn_out.write (cols[0] + ' ' + startkey + '\n')
f_txn.close()
f_txn_out.close()

cmd = 'rm -f ' + out_ycsb_load
os.system(cmd)
cmd = 'rm -f ' + out_ycsb_txn
os.system(cmd)

#####################################################################################

if key_type == 'zipfian' or key_type == 'uniform' or key_type == 'mono' :
    print bcolors.OKGREEN + 'generating random integers' + bcolors.ENDC
    # f_load = open (out_load_ycsbkey, 'r')
    # f_load_out = open (out_load, 'w')
    # for line in f_load :
    #     f_load_out.write (line)

    # f_txn = open (out_txn_ycsbkey, 'r')
    # f_txn_out = open (out_txn, 'w')
    # for line in f_txn :
    #     f_txn_out.write (line)
    cmd = 'cp ' + out_load_ycsbkey + ' ' + out_load
    os.system(cmd)
    cmd = 'cp ' + out_txn_ycsbkey + ' ' + out_txn
    os.system(cmd)

elif key_type == 'string' :
    print bcolors.OKGREEN + 'generating random strings from UUIDs' + bcolors.ENDC
    keymap = {}
    f_load = open (out_load_ycsbkey, 'r')
    f_load_out = open (out_load, 'w')
    for line in f_load :
        cols = line.split()
        keymap[int(cols[1])] = random_string(31)
        f_load_out.write (cols[0] + ' ' + keymap[int(cols[1])] + '\n')

    f_txn = open (out_txn_ycsbkey, 'r')
    f_txn_out = open (out_txn, 'w')
    for line in f_txn :
        cols = line.split()
        if cols[0] == 'SCAN' :
            f_txn_out.write (cols[0] + ' ' + keymap[int(cols[1])] + ' ' + cols[2] + '\n')
        elif cols[0] == 'INSERT' :
            keymap[int(cols[1])] = random_string(31)
            f_txn_out.write (cols[0] + ' ' + keymap[int(cols[1])] + '\n')
        else :
            f_txn_out.write (cols[0] + ' ' + keymap[int(cols[1])] + '\n')

elif key_type == 'uuid' :
    print bcolors.OKGREEN + 'generating random UUIDs' + bcolors.ENDC
    keymap = {}
    f_load = open (out_load_ycsbkey, 'r')
    f_load_out = open (out_load, 'w')
    for line in f_load :
        cols = line.split()
        uuid_int = uuid.uuid4().bytes
        uuid_str = reduce(lambda z1, z2: z1 + " " + str(ord(z2)), uuid_int, "")
        keymap[int(cols[1])] = uuid_str
        f_load_out.write (cols[0] + ' ' + uuid_str + '\n')

    f_txn = open (out_txn_ycsbkey, 'r')
    f_txn_out = open (out_txn, 'w')
    for line in f_txn :
        cols = line.split()
        if cols[0] == 'SCAN' :
            f_txn_out.write (cols[0] + ' ' + str(keymap[int(cols[1])]) + ' ' + cols[2] + '\n')
        elif cols[0] == 'INSERT' :
            uuid_int = uuid.uuid4().bytes
            uuid_str = reduce(lambda z1, z2: z1 + " " + str(ord(z2)), uuid_int, "")
            keymap[int(cols[1])] = uuid_int
            f_txn_out.write (cols[0] + ' ' + uuid_str + '\n')
        else :
            f_txn_out.write (cols[0] + ' ' + str(keymap[int(cols[1])]) + '\n')

# elif key_type == 'monoint' :
#     print bcolors.OKGREEN + 'generating monotonously increasing integers' + bcolors.ENDC
#     keymap = {}
#     f_load = open (out_load_ycsbkey, 'r')
#     f_load_out = open (out_load, 'w')
#     count = 0
#     for line in f_load :
#         cols = line.split()
#         keymap[int(cols[1])] = count
#         f_load_out.write (cols[0] + ' ' + str(count) + '\n')
#         count += 1

#     f_txn = open (out_txn_ycsbkey, 'r')
#     f_txn_out = open (out_txn, 'w')
#     for line in f_txn :
#         cols = line.split()
#         if cols[0] == 'SCAN' :
#             f_txn_out.write (cols[0] + ' ' + str(keymap[int(cols[1])]) + ' ' + cols[2] + '\n')
#         elif cols[0] == 'INSERT' :
#             keymap[int(cols[1])] = count
#             f_txn_out.write (cols[0] + ' ' + str(count) + '\n')
#             count += 1
#         else :
#             f_txn_out.write (cols[0] + ' ' + str(keymap[int(cols[1])]) + '\n')

elif key_type == 'email' :
    print bcolors.OKGREEN + 'generating emails' + bcolors.ENDC
    keymap = {}
    f_email = open (email_list, 'r')
    emails = f_email.readlines()

    f_load = open (out_load_ycsbkey, 'r')
    f_load_out = open (out_load, 'w')

    sample_size = len(f_load.readlines())
    gap = email_list_size / sample_size

    f_load.close()
    f_load = open (out_load_ycsbkey, 'r')
    count = 0
    for line in f_load :
        cols = line.split()
        email = reverseHostName(emails[count * gap])
        keymap[int(cols[1])] = email
        f_load_out.write (cols[0] + ' ' + email + '\n')
        count += 1

    count = 0
    f_txn = open (out_txn_ycsbkey, 'r')
    f_txn_out = open (out_txn, 'w')
    for line in f_txn :
        cols = line.split()
        if cols[0] == 'SCAN' :
            f_txn_out.write (cols[0] + ' ' + keymap[int(cols[1])] + ' ' + cols[2] + '\n')
        elif cols[0] == 'INSERT' :
            email = reverseHostName(emails[count * gap + 1])
            keymap[int(cols[1])] = email
            f_txn_out.write (cols[0] + ' ' + email + '\n')
            count += 1
        else :
            f_txn_out.write (cols[0] + ' ' + keymap[int(cols[1])] + '\n')

f_load.close()
f_load_out.close()
f_txn.close()
f_txn_out.close()

cmd = 'rm -f ' + out_load_ycsbkey
os.system(cmd)
cmd = 'rm -f ' + out_txn_ycsbkey
os.system(cmd)
