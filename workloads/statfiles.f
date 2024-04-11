# Creates a fileset of $nfiles number of files, then loops through them
# using $nthreads number of threads, doing "stat" calls on each file.

set $dir=/home/parallels/Developer/hybridfs/mntdir
set $nfiles=10000
set $meandirwidth=20
set $filesize=0
set $nthreads=20

define fileset name=bigfileset,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=examinefiles,instances=1
{
  thread name=examinefilethread, memsize=10m,instances=$nthreads
  {
    flowop statfile name=statfile1,filesetname=bigfileset
  }
}

run 60
