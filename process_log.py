from collections import defaultdict
import sys, os.path

if len(sys.argv) < 2 or not os.path.isdir(sys.argv[1]):
    print "Usage: <log_directory>"
    exit(0)

iter_times = defaultdict(lambda: defaultdict(int))

for f_name in os.listdir(sys.argv[1]):
    with open(sys.argv[1] + f_name, 'r') as f:
        for line in f:
            if "[STAT_ITER]" not in line: continue

            iter_num, tid, iter_time = map(int, line[line.index("[STAT_ITER]") +\
                len("[STAT_ITER]"):].split(','))
            iter_times[iter_num][tid] += iter_time

#with open(sys.argv[1][:-1].replace('/', ',') + '.sum_iter_times', 'w') as f:
for iter_num in iter_times:
    print "{0:d} {1:d}".format(iter_num, sum(iter_times[iter_num].values()))

