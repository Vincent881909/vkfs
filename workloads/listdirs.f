# Creates a fileset with a fairly deep directory tree, then does readdir
# operations on them for a specified amount of time.

set $dir=/home/parallels/Developer/hybridfs/mntdir
set $nfiles=50000
set $meandirwidth=5
set $nthreads=16

define fileset name=bigfileset,path=$dir,size=0,entries=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=lsdir,instances=1
{
  thread name=dirlister,memsize=1m,instances=$nthreads
  {
    flowop listdir name=open1,filesetname=bigfileset
  }
}

run 60
