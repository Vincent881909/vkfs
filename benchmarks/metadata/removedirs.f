
set $dir=mntdir
set $ndirs=100000
set $meandirwidth=100
set $nthreads=1

set mode quit firstdone

define fileset name=bigfileset,path=$dir,size=4,leafdirs=$ndirs,dirwidth=$meandirwidth,prealloc

define process name=remdir,instances=1
{
  thread name=removedirectory,memsize=1m,instances=$nthreads
  {
    flowop removedir name=dirremover,filesetname=bigfileset
  }
}

run
