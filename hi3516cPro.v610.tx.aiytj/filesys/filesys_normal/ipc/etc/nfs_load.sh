#!/bin/sh

if [ $# -eq 0 ]; then
    mode="-i"
else
    mode=$1
fi

function load_nfs()
{
    echo "loading nfs ko ..."
    insmod /ipc/drv/sunrpc.ko
    insmod /ipc/drv/grace.ko
    insmod /ipc/drv/lockd.ko
    insmod /ipc/drv/nfs.ko
    insmod /ipc/drv/nfsv2.ko
    insmod /ipc/drv/nfsv3.ko
    echo "load nfs ko Done!!!"
}

function remove_nfs()
{
    echo "removing nfs ko ..."
    rmmod nfsv3
    rmmod nfsv2
    rmmod nfs
    rmmod lockd
    rmmod grace
    rmmod sunrpc
    echo "remove nfs Done!!!"
}

function nfs()
{
    if [ "$mode" == "-i" ]; then
        load_nfs
    elif [ "$mode" == "-r" ]; then
        remove_nfs
    else
        echo "[error] Invalid param, please use the following parameters"
        echo "-i:  insmod"
        echo "-r:  rmmod"
    fi
}

nfs
