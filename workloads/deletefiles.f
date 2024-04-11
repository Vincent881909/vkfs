# Fire off 16 threads ($nthreads), where each thread stops after
# deleting 1000 ($count) files.

set $dir=/home/parallels/Developer/hybridfs/mntdir
set $count=1000
set $filesize=16k
set $nfiles=5000
set $meandirwidth=100
set $nthreads=16

set mode quit firstdone

define fileset name=bigfileset,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100,paralloc

define process name=filedelete,instances=1
{
  thread name=filedeletethread,memsize=10m,instances=$nthreads
  {
    flowop deletefile name=deletefile1,filesetname=bigfileset
    flowop opslimit name=limit
    flowop finishoncount name=finish,value=$count
  }
}

run
