#!/usr/local/bin/bash

##  run this before making the kernel, between step 4 and 5
##  dropbear doesn't run be default, this is by design,  you need to go to /exec.php 
##  and execute 
##    /etc/rc.dropbear &
##  after making it executable

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
	cp /usr/local/bin/dropbearkey $MW_BUILDPATH/t1n1fs/usr/local/bin/dropbearkey
fi

cp /usr/sbin/pw $MW_BUILDPATH/t1n1fs/usr/sbin
cp $MW_BUILDPATH/freebsd10/build/misc/rc.dropbear $MW_BUILDPATH/t1n1fs/etc      

if [ ! -e $MW_BUILDPATH/usr/local/bin/ksh ]; then
	cp /usr/local/bin/ksh $MW_BUILDPATH/t1n1fs/usr/local/bin/ksh
fi

cp /usr/bin/nc $MW_BUILDPATH/us/bin/nc
cp /usr/sbin/tcpdump $MW_BUILDPATH/usr/sbin/tcpdump

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
cp /boot/kernel/dtraceall.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/opensolaris.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/dtrace.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/dtmalloc.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/dtnfscl.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/fbt.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/fasttrap.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/lockstat.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/sdt.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/systrace.ko $MW_BUILDPATHt1n1fs/boot/kernel/
cp /boot/kernel/profile.ko $MW_BUILDPATHt1n1fs/boot/kernel/

# make libs
        cd $MW_BUILDPATH/tmp
        perl $MW_BUILDPATH/freebsd10/build/minibsd/mklibs.pl $MW_BUILDPATH/t1n1fs > t1n1wall.libs
        perl $MW_BUILDPATH/freebsd10/build/minibsd/mkmini.pl t1n1wall.libs / $MW_BUILDPATH/t1n1fs
