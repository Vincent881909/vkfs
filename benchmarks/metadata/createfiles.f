
set $dir=/home/parallels/Developer/hybridfs/mntdir
set $nfiles=50000
set $meandirwidth=100
set $meanfilesize=4k
set $iosize=1m
set $nthreads=1

set mode quit firstdone

define fileset name=bigfileset,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth

define process name=filecreate,instances=1
{
  thread name=filecreatethread,memsize=10m,instances=$nthreads
  {
    flowop createfile name=createfile1,filesetname=bigfileset,fd=1
    flowop writewholefile name=writefile1,fd=1,iosize=$iosize
    flowop closefile name=closefile1,fd=1
  }
}

run
