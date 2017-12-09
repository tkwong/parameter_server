#!/usr/bin/env python

import os
from os.path import dirname, join

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
hostfile = "machinefiles/local"
#hostfile = "machinefiles/2node"
# hostfile = "machinefiles/5node"
# hostfile = "machinefiles/6node"
progfile = "debug/LDAExample"

script_path = os.path.realpath(__file__)
proj_dir = dirname(dirname(script_path))
print "flexps porj_dir:", proj_dir
hostfile_path = join(proj_dir, hostfile)
prog_path = join(proj_dir, progfile)
print "hostfile_path:%s, prog_path:%s" % (hostfile_path, prog_path)

params = {
    "config_file" : hostfile_path,
    "hdfs_namenode" : "proj10",
    "hdfs_namenode_port" : 9000,
    #"input" : "hdfs:///1155004171/dataset/preprocess_kos.txt",
    #"input" : "/data/opt/tmp/1155004171/dataset/kos/preprocess_kos.txt",
    #"max_voc_id" : 6906,
    #"num_docs" : 3430,
    #"input" : "hdfs:///1155004171/dataset/preprocess_nytimes.txt",
    "input" : "data/preprocess_nytimes.txt",
    "max_voc_id" : 102660,
    "num_docs" : 299752,
    #"input" : "hdfs:///1155004171/dataset/preprocess_pubmed.txt",
    #"max_voc_id" : 141043,
    #"kStaleness" : 0,
    "kStaleness" : 3,
    "kModelType" : "BSP",  # {ASP/SSP/BSP}
    "kStorageType" : "Vector",  # {Vector/Map}
    "num_dims" : 54686452,
    "batch_size" : 1,
    "max_vocs_each_pull" : 102660,
    #"num_batches" : 50,
    "num_batches" : 1,
    #"num_workers_per_node" : 10,
    "num_workers_per_node" : 1,
    "num_servers_per_node" : 1,
    "alpha" : 0.1, # learning rate
    "num_topics" : 10,
    "num_iters" : 5,
    "compute_loglikehood": 0
}

ssh_cmd = (
    "ssh "
    "-o StrictHostKeyChecking=no "
    "-o UserKnownHostsFile=/dev/null "
    )

env_params = (
  "GLOG_logtostderr=true "
  "GLOG_v=-1 "
  "GLOG_minloglevel=0 "
  # this is to enable hdfs short-circuit read (disable the warning info)
  # change this path accordingly when we use other cluster
  # the current setting is for proj5-10
  "LIBHDFS3_CONF=/data/opt/course/hadoop/etc/hadoop/hdfs-site.xml"
  )

# TODO: May need to ls before run to make sure the related files are synced.
#clear_cmd = "ls " + hostfile_path + " > /dev/null; ls " + prog_path + " > /dev/null; ulimit -c unlimited; "
clear_cmd = "ls " + dirname(hostfile_path) + " > /dev/null; ls " + dirname(prog_path) + " > /dev/null; "

with open(hostfile_path, "r") as f:
  hostlist = []
  hostlines = f.read().splitlines()
  for line in hostlines:
    if not line.startswith("#"):
      hostlist.append(line.split(":"))

  for [node_id, host, port] in hostlist:
    print "node_id:%s, host:%s, port:%s" %(node_id, host, port)
    cmd = ssh_cmd + host + " "  # Start ssh command
    cmd += "\""  # Remote command starts
    cmd += clear_cmd
    # Command to run program
    cmd += env_params + " " + prog_path
    cmd += " --my_id="+node_id
    cmd += "".join([" --%s=%s" % (k,v) for k,v in params.items()])

    cmd += "\""  # Remote Command ends
    cmd += " &"
    print cmd
    os.system(cmd)