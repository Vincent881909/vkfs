# Creates a fileset with $ndirs empty leaf directories then rmdir's all of them

set $dir=/home/parallels/Developer/hybridfs/mntdir
set $ndirs=10000
set $meandirwidth=100
set $nthreads=16

set mode quit firstdone

define fileset name=bigfileset,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth,prealloc

define process name=remdir,instances=1
{
  thread name=removedirectory,memsize=1m,instances=$nthreads
  {
    flowop removedir name=dirremover,filesetname=bigfileset
  }
}

run
