#!/usr/bin/env python

import os
from os.path import dirname, join
import subprocess
import time
import glob
from random import randint

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

script_path = os.path.realpath(__file__)
proj_dir = dirname(dirname(script_path))
print "proj_dir:", proj_dir

global_log_dir = proj_dir + "/logs/" + str(int(time.time()))
os.mkdir(global_log_dir)

for update_rate in [1,2,3,5,10,20] :
    for perm_straggler in [True, False] :
        for tran_straggler in [True, False] :
            for delay_percent in [100,500,1000] :

                hostfile = "machinefiles/5node"
                #hostfile = "machinefiles/2node"
                progfile = "build/LogisticRegression"

                prog_path = join(proj_dir, progfile)
                hostfile_path = join(proj_dir, hostfile)
                print "hostfile_path:%s, prog_path:%s" % (hostfile_path, prog_path)
                params = {
                    "config_file":hostfile_path,
                    "hdfs_master_port": randint(10000,60000),
                    "input": "hdfs://proj10:9000/datasets/classification/avazu-app-part/",
                    "n_features": 1000000,
                    "n_iters": 100,
                    #"n_iters": 10,
                    "batch_size": 100,
                    #"input": "hdfs://proj10:9000/datasets/classification/a9/",
                    #"n_features": 123,
                    "n_workers_per_node": 5,
                    #"with_injected_straggler": 0.05,
                    "with_injected_straggler_delay_percent": delay_percent,
                    #"get_updated_workload_rate": 1,
                    "get_updated_workload_rate": update_rate,
                    "activate_scheduler": True,
                    "activate_transient_straggler": tran_straggler,
                    "activate_permanent_straggler": perm_straggler,
                    "prefetch_model_before_batch" : False

                }

                job_name = "update_" + str(update_rate) + "_perm_" + str(perm_straggler) + "_tran_" + str(tran_straggler) + "_delay_" + str(delay_percent)
                log_dir = global_log_dir + "/" + job_name
                os.mkdir(log_dir)

                ssh_cmd = (
                    "ssh "
                    "-o StrictHostKeyChecking=no "
                    "-o UserKnownHostsFile=/dev/null "
                    )

                env_params = (
                  "GLOG_logtostderr=true "
                  "GLOG_v=2 "
                  #"GLOG_minloglevel=0 "
                  # this is to enable hdfs short-circuit read (disable the warning info)
                  # change this path accordingly when we use other cluster
                  # the current setting is for proj5-10
                  "LIBHDFS3_CONF=/data/opt/course/hadoop/etc/hadoop/hdfs-site.xml"
                  )

                # TODO: May need to ls before run to make sure the related files are synced.
                clear_cmd = "ls " + hostfile_path + " > /dev/null ; ls " + prog_path + " > /dev/null; "
                proc_list = []
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
                    #cmd += " 2>&1 | tee " + log_dir + "/output." + node_id + ".log \" "
                    cmd += " >> " + log_dir + "/output." + node_id + ".log 2>&1 \" "
                    print cmd
                    #os.system(cmd)
                    proc_list.append(subprocess.Popen(cmd, shell=True))

                for proc in proc_list:
                    proc.wait()

                print "Finished: All Worker stopped. output in " + log_dir

                print "Preparing statistic file"
                stat_types=['wait' , 'proc', 'iter', 'work', 'time', 'totl']
                for type in stat_types :
                    os.system("grep STAT_" + type.upper() + " " + log_dir + "/output.*.log | awk '{print \"" + job_name + "\" \",\" $5 \",\" $6}' > " + log_dir + "/"+job_name+"_stat_" + type + ".csv")

email=os.getlogin() + "@link.cuhk.edu.hk"
print "Sending Email to [" + email + "]"
attachments = ""

for file in glob.glob(global_log_dir + "/**/*.csv"):
    print "attaching " + file
    attachments += " -a " + file
os.system("mail -s 'Result for" + log_dir + "' " + attachments +  " " + email + " < /dev/null")

