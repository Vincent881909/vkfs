# Creates a fileset of $nfiles number of files, then loops through them
# using $nthreads number of threads, doing "stat" calls on each file.

set $dir=mntdir
set $nfiles=10000
set $meandirwidth=6
set $filesize=3k
set $nthreads=8

define fileset name=bigfileset,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=examinefiles,instances=1
{
  thread name=examinefilethread, memsize=10m,instances=$nthreads
  {
    flowop statfile name=statfile1,filesetname=bigfileset
  }
}

run 10
