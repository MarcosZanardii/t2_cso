#!/bin/sh

set -e

SIZE=128
N=10

modprobe pubsub_driver max_msg_size=$SIZE max_msg_n=$N
/bin/test_pubsub_driver