
set $dir=mntdir
set $nfiles=50000
set $meandirwidth=5
set $nthreads=1
set $meanfilesize=4k

define fileset name=bigfileset,path=$dir,size=$meanfilesize,entries=$nfiles,dirwidth=$meandirwidth,prealloc

define process name=lsdir,instances=1
{
  thread name=dirlister,memsize=1m,instances=$nthreads
  {
    flowop listdir name=open1,filesetname=bigfileset
  }
}

run 30

