#!/usr/bin/env python

import os
from os.path import dirname, join
import subprocess
import time

# The file path should be related path from the project home path.
# The hostfile should be in the format:
# node_id:hostname:port
#
# Example:
# 0:worker1:37542
# 1:worker2:37542
# 2:worker3:37542
# 3:worker4:37542
# 4:worker5:37542
#
#hostfile = "machinefiles/local"
hostfile = "machinefiles/5node"
#hostfile = "machinefiles/2node"
progfile = "build/MatrixFac"

script_path = os.path.realpath(__file__)
proj_dir = dirname(dirname(script_path))
print "proj_dir:", proj_dir
hostfile_path = join(proj_dir, hostfile)
prog_path = join(proj_dir, progfile)
print "hostfile_path:%s, prog_path:%s" % (hostfile_path, prog_path)

params = {
    "config_file":hostfile_path,
    #"input": "hdfs://proj10:9000/datasets/ml/yahoomusic",
    #"n_users": 1800000,
    #"n_items": 136000000,
    "n_iters": 1,
    "batch_size": 10,
    "input": "hdfs://proj10:9000/datasets/ml/netflix/",
    "n_users": 2649430,
    "n_items": 17771,
    "n_workers_per_node": 1,
    "alpha": 0.1,
    "beta": 0.005,
    "latent": 20,
    #"with_injected_straggler": 0.05,
    "with_injected_straggler_delay": 10,
    "get_updated_workload_rate": 5,
    "activate_scheduler": True,
    "activate_transient_straggler": False,
    "activate_permanent_straggler": False
}

ssh_cmd = (
    "ssh "
    "-o StrictHostKeyChecking=no "
    "-o UserKnownHostsFile=/dev/null "
    )

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1"
  #"GLOG_minloglevel=0 "
  # this is to enable hdfs short-circuit read (disable the warning info)
  # change this path accordingly when we use other cluster
  # the current setting is for proj5-10
  "LIBHDFS3_CONF=/data/opt/course/hadoop/etc/hadoop/hdfs-site.xml"
  )

# TODO: May need to ls before run to make sure the related files are synced.
clear_cmd = "ls " + hostfile_path + " > /dev/null ; ls " + prog_path + " > /dev/null; "
#log_dir = proj_dir + "/logs/" + str(int(time.time()))
#os.mkdir(log_dir)
#proc_list = []
with open(hostfile, "r") as f:
  hostlist = []
  hostlines = f.read().splitlines()
  for line in hostlines:
    if not line.startswith("#"):
      hostlist.append(line.split(":"))

  for [node_id, host, port] in hostlist:
    print "node_id:%s, host:%s, port:%s" %(node_id, host, port)
    cmd = ssh_cmd + host + " \""
    # cmd += clear_cmd
    cmd += "env " + env_params + " " + prog_path
    cmd += " --my_id="+node_id
    cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])
    #cmd += " >> " + log_dir + "/output." + node_id + ".log 2>&1 \" "
    cmd += " \" &"
    print cmd
    os.system(cmd)
    #proc_list.append(subprocess.Popen(cmd, shell=True))


exit(0)


while len(os.listdir(log_dir)) <= 0 :
    time.sleep(1)

tail_proc = subprocess.Popen("tail -f " + log_dir + "/output.*", shell=True)

for proc in proc_list:
    proc.wait()

tail_proc.terminate()

print
print "Finished: All Worker stopped. output in " + log_dir

print "Preparing statistic file"

os.system("grep STAT_WAIT " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_wait.csv")
os.system("grep STAT_PROC " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_proc.csv")
os.system("grep STAT_ITER " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_iter.csv")
os.system("grep STAT_WORK " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_work.csv")
os.system("grep STAT_TIME " + log_dir + "/output.* | awk '{print $5 \",\" $6}' > " + log_dir + "/stat_time.csv")

email=os.getlogin() + "@link.cuhk.edu.hk"
print "Email result? [" + email + "]"
yes = {'yes','y', 'ye', ''}
no = {'no','n'}
choice = raw_input().lower()
if choice in yes :
    os.system("mail -s 'Result for" + log_dir + "' -a " + log_dir + "/stat_iter.csv   -a " + log_dir + "/stat_proc.csv   -a " + log_dir + "/stat_time.csv   -a " + log_dir + "/stat_wait.csv   -a " + log_dir + "/stat_work.csv  "+email+" < /dev/null")
elif choice in no:
    print ""
else:
  print "Please respond with 'yes' or 'no'"

print "Finished"

