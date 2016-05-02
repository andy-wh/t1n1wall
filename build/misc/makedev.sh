#!/usr/local/bin/bash

set -e

if [ -z "$MW_BUILDPATH" -o ! -d "$MW_BUILDPATH" ]; then
        echo "\$MW_BUILDPATH is not set"
        exit 1
fi

# install prerequesites

if [ ! -e /usr/local/sbin/dropbear ]; then
	pkg install -y -g 'dropbear'
fi

if [ ! -e /usr/local/bin/ksh ]; then
        pkg install -y -g 'pdksh'
fi


## install dev tools

if [ ! -e $MW_BUILDPATH/t1n1fs/usr/local/sbin/dropbear ]; then
	cp /usr/local/sbin/dropbear $MW_BUILDPATH/t1n1fs/usr/local/sbin/dropbear
	cp /usr/local/dropbearkey $MW_BUILDPATH/t1n1fs/usr/local/bin/dropbearkey
fi

cp $MW_BUILDPATH/freebsd10/build/misc/rc.dropbear $MW_BUILDPATH/t1n1fs/etc      

if [ ! -e $MW_BUILDPATH/usr/local/bin/ksh ]; then
	cp /usr/local/bin/ksh $MW_BUILDPATH/t1n1fs/usr/local/bin/ksh
fi

## add dtrace to kernel
if [ ! -e $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386.orig ]; then
	cp $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386 $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386.orig
fi

cp $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386.orig $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386
 
echo "
options         KDTRACE_HOOKS
options         DDB_CTF
makeoptions	DEBUG=-g
makeoptions	WITH_CTF=1
options         NFSCL                   # New Network Filesystem Client
options         NFSD                    # New Network Filesystem Server
options         NFSLOCKD                # Network Lock Manager
options         NFS_ROOT                # NFS usable as /, requires NFSCL

" >> $MW_BUILDPATH/freebsd10/build/kernelconfigs/T1N1WALL_GENERIC.i386 

## install dtrace

cp /usr/sbin/dtrace $MW_BUILDPATH/t1n1fs/usr/sbin/dtrace
cp /boot/kernel/dtraceall.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/opensolaris.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/dtrace.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/dtmalloc.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/dtnfscl.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/fbt.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/fasttrap.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/lockstat.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/sdt.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/systrace.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/
cp /boot/kernel/profile.ko /usr/t1n1wall/build10/t1n1fs/boot/kernel/




# make libs
        cd $MW_BUILDPATH/tmp
        perl $MW_BUILDPATH/freebsd10/build/minibsd/mklibs.pl $MW_BUILDPATH/t1n1fs > t1n1wall.libs
        perl $MW_BUILDPATH/freebsd10/build/minibsd/mkmini.pl t1n1wall.libs / $MW_BUILDPATH/t1n1fs
