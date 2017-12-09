import sys, os.path
from collections import defaultdict

if len(sys.argv) < 2 or not os.path.isfile(sys.argv[1]):
    print "Usage: <log_file>"
    exit(0)
    
iter_times = defaultdict(int)

with open(sys.argv[1], 'r') as f:
    for line in f:
        iter_num, tid, iter_time = map(int, line.split(',')[1:])
        iter_times[iter_num] += iter_time
            
with open(sys.argv[1].replace('/', '_') + '.processed', 'w') as out_f:
    for iter_num, iter_time in iter_times.items():
        out_f.write('{0:d} {1:d}\n'.format(iter_num, iter_time))
