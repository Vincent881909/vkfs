set $dir=mntdir
set $nfiles=100000
set $meandirwidth=20
set $filesize=0
set $nthreads=1

define fileset name=bigfileset,path=$dir,size=$filesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc=100

define process name=examinefiles,instances=1
{
  thread name=examinefilethread, memsize=10m,instances=$nthreads
  {
    flowop statfile name=statfile1,filesetname=bigfileset
  }
}

run 60
