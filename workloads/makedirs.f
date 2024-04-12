# Creates a directory with $ndirs potential leaf directories, than mkdir's them

set $dir=/home/parallels/Developer/hybridfs/mntdir
set $ndirs=100000
set $meandirwidth=100
set $nthreads=16

set mode quit firstdone

define fileset name=bigfileset,path=$dir,size=0,leafdirs=$ndirs,dirwidth=$meandirwidth

define process name=dirmake,instances=1
{
  thread name=dirmaker,memsize=1m,instances=$nthreads
  {
    flowop makedir name=mkdir1,filesetname=bigfileset
  }
}

run
