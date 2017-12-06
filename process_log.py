from collections import defaultdict
import sys, os.path

if len(sys.argv) < 2 or not os.path.isfile(sys.argv[1]):
    print "Usage: <log_filename>"
    exit(0)

with open(sys.argv[1], 'r') as f:
    iter_times = defaultdict(list)

    for line in f:
        if "STAT_ITER" not in line: continue

        iter_num, tid, iter_time = map(int, line[line.index("[STAT_ITER]") + len("[STAT_ITER]"):]\
            .split(','))
        iter_times[tid].append(iter_time)

for tid in iter_times:
    with open(str(tid) + '_iter_time', 'w') as f:
        for iter_num, iter_time in enumerate(iter_times[tid]):
            f.write('{0:d} {1:d}\n'.format(iter_num, iter_time))

