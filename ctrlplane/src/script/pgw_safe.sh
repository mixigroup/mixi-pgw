#!/bin/sh

## ex) /opt/pgw/mixi_pgw_ctrl_plane <- executable
PIDDIR=/opt/pgw/pid
PIDFILE=${PIDDIR}/mixi_pgw_ctrl_plane.pid
if [ $1 ]; then
  PIDFILE=${PIDDIR}/$1
fi
while true
do
  PID=`cat ${PIDFILE}`
  num=`ps -p ${PID} |wc -l`
  if [ $num -lt 2 ]; then
    ## remove pid file
    echo "start mixi_pgw_ctrl_plane"
    rm -f ${PIDFILE}
    /opt/pgw/mixi_pgw_ctrl_plane -d -p ${PIDFILE}
    sleep 3
  fi
  sleep 1
done
