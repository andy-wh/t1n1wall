#!/usr/local/bin/bash

set -e

if [ -z "$MW_BUILDPATH" -o ! -d "$MW_BUILDPATH" ]; then
	echo "\$MW_BUILDPATH is not set"
	exit 1
fi

export CC=gcc46

# set working directory etc for ports compilation
	rm -Rf $MW_BUILDPATH/tmp/ports/work
	mkdir -p $MW_BUILDPATH/tmp/ports/work
        export PORTSDIR=$MW_BUILDPATH/tmp/ports/tree
# set port options for ports that need user input
	rm -Rf $MW_BUILDPATH/tmp/ports/db
	mkdir -p $MW_BUILDPATH/tmp/ports/db
	export PORT_DBDIR=$MW_BUILDPATH/tmp/ports/db
	
	for portoptf in $MW_BUILDPATH/freebsd11/build/files/portoptions/* ; do
		port=${portoptf##*/}
		mkdir -p $PORT_DBDIR/$port
		cp $portoptf $PORT_DBDIR/$port/options
	done


######## manually compiled packages ########

# select Autoconf version 2.13
		export AUTOCONF_VERSION=2.13
# php 4.4.9
	cd $MW_BUILDPATH/tmp
	rm -Rf php-4.4.9
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/php-4.4.9.tar.bz2
        cd php-4.4.9/ext/
	tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/radius-1.2.5.tgz
        mv radius-1.2.5 radius
        cd ..
	rm configure
        AUTOCONF_VERSION=2.13 ./buildconf --force
        ./configure --without-mysql --with-pear --with-openssl --enable-discard-path --enable-radius --enable-sockets --enable-bcmath
        patch < $MW_BUILDPATH/freebsd11/build/patches/packages/php.openssl.c.patch
        patch < $MW_BUILDPATH/freebsd11/build/patches/packages/php.fopencookie.diff
        make
        install -s sapi/cgi/php $MW_BUILDPATH/t1n1fs/usr/local/bin/
# mini httpd
        cd $MW_BUILDPATH/tmp
        rm -Rf mini_httpd-1.25
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/mini_httpd-1.25.tar.gz
        cd mini_httpd-1.25/
        patch < $MW_BUILDPATH/freebsd11/build/patches/packages/mini_httpd.patch
        make
        install -s mini_httpd $MW_BUILDPATH/t1n1fs/usr/local/sbin
# ezipupdate
        cd $MW_BUILDPATH/tmp
	rm -Rf ez-ipupdate-3.0.11b8
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/ez-ipupdate-3.0.11b8.tar.gz
        cd ez-ipupdate-3.0.11b8
        patch < $MW_BUILDPATH/freebsd11/build/patches/packages/ez-ipupdate.c.patch
        ./configure
        make
        install -s ez-ipupdate $MW_BUILDPATH/t1n1fs/usr/local/bin/
# modem-stats
	cd $MW_BUILDPATH/tmp
	rm -Rf modem-stats-1.0.1
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/modem-stats-1.0.1.src.elf.tar.gz
	cd modem-stats-1.0.1
	patch < $MW_BUILDPATH/freebsd11/build/patches/user/modem-stats.c.patch
	make
	install -s modem-stats $MW_BUILDPATH/t1n1fs/sbin
# dnsmasq
        cd $MW_BUILDPATH/tmp
        rm -Rf dnsmasq-2.76
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/dnsmasq-2.76.tar.gz
        cd dnsmasq-2.76
        cp $MW_BUILDPATH/freebsd11/build/patches/packages/patch-dnsmasq-iscreader.patch .
        # patch < patch-dnsmasq-iscreader.patch
        make COPTS+=-DNO_AUTH COPTS+=-DNO_TFTP COPTS+=-DNO_SCRIPT COPTS+=-DNO_LARGEFILE 
        install -s src/dnsmasq $MW_BUILDPATH/t1n1fs/usr/local/sbin
        rm patch-dnsmasq-iscreader.patch
# dudders
        cd $MW_BUILDPATH/tmp
        rm -Rf dudders-1.04
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/dudders-1.04.tar.bz2
        cd dudders-1.04
        ./configure --with-crypto=openssl CFLAGS="-DNDEBUG -O2"
        make
        install -s dudders $MW_BUILDPATH/t1n1fs/usr/local/bin
        
######## FreeBSD ports ########
# ntpd
        export CC=cc
 	cd $PORTSDIR/net/openntpd
        make CONFIGURE_ARGS="--with-privsep-user=root --localstatedir=/var" MASTER_SITE_OVERRIDE=http://ftp.heanet.ie/mirrors/OpenBSD/OpenNTPD/
	export CC=gcc46
# ISC dhcp-client
	cd $PORTSDIR/net/isc-dhcp43-client
	make
	install -s $WRKDIRPREFIX/$PORTSDIR/net/isc-dhcp43-client/work/dhcp-*/client/dhclient $MW_BUILDPATH/t1n1fs/sbin/
# ipsec-tools
        cd $PORTSDIR/security/ipsec-tools
        make
        install -s $WRKDIRPREFIX/$PORTSDIR/security/ipsec-tools/work/ipsec-tools-*/src/racoon/.libs/racoon $MW_BUILDPATH/t1n1fs/usr/local/sbin
        install -s $WRKDIRPREFIX/$PORTSDIR/security/ipsec-tools/work/ipsec-tools-*/src/setkey/.libs/setkey $MW_BUILDPATH/t1n1fs/usr/local/sbin
        install -s $WRKDIRPREFIX/$PORTSDIR/security/ipsec-tools/work/ipsec-tools-*/src/libipsec/.libs/libipsec.so.0 $MW_BUILDPATH/t1n1fs/usr/local/lib
# strongswan
#       cd $PORTSDIR/security/strongswan
#       make
#       install -s $WRKDIRPREFIX/$PORTSDIR/security/strongswan/work/stage/usr/local/sbin/swanctl $MW_BUILDPATH/t1n1fs/usr/local/sbin
#	install $WRKDIRPREFIX/$PORTSDIR/security/strongswan/work/stage/usr/local/sbin/ipsec $MW_BUILDPATH/t1n1fs/usr/local/sbin
#	install -s $WRKDIRPREFIX/$PORTSDIR/security/strongswan/work/stage/usr/local/sbin/charon-cmd $MW_BUILDPATH/t1n1fs/usr/local/sbin
# dhcp6
	cd $PORTSDIR/net/dhcp6
        make
	install -s $WRKDIRPREFIX/$PORTSDIR/net/dhcp6/work/wide-dhc*/dhcp6c $MW_BUILDPATH/t1n1fs/usr/local/sbin
	install -s $WRKDIRPREFIX/$PORTSDIR/net/dhcp6/work/wide-dhc*/dhcp6s $MW_BUILDPATH/t1n1fs/usr/local/sbin
# mpd5
	cd $PORTSDIR/net/mpd5
        make
	# remove PAM
	cd $PORTSDIR/net/mpd5/work/mpd-5.*/src
	make clean
	sed -i '' -e's/^USE_AUTH_PAM/#USE_AUTH_PAM/' Makefile
	patch < $MW_BUILDPATH/freebsd11/build/patches/packages/mpd5.backtrace.patch
	make
	install -s $WRKDIRPREFIX/$PORTSDIR/net/mpd5/work/mpd-*/src/mpd5 $MW_BUILDPATH/t1n1fs/usr/local/sbin/
	mv Makefile.orig Makefile
# xmbmon
	cd $PORTSDIR/sysutils/xmbmon
        make
	install -s $WRKDIRPREFIX/$PORTSDIR/sysutils/xmbmon/work/xmbmon*/mbmon $MW_BUILDPATH/t1n1fs/usr/local/bin/
# wol
	cd $PORTSDIR/net/wol
	patch < $MW_BUILDPATH/freebsd11/build/patches/packages/wol.makefile.patch
	make
        install -s $WRKDIRPREFIX/$PORTSDIR/net/wol/work/wol-*/src/wol $MW_BUILDPATH/t1n1fs/usr/local/bin/
	mv Makefile.orig Makefile

# make t1n1wall tools and binaries
        cd $MW_BUILDPATH/tmp
        cp -r $MW_BUILDPATH/freebsd11/build/tools .
        cd tools
        gcc -o stats.cgi stats.c
        gcc -o minicron minicron.c
        gcc -o choparp choparp.c
        gcc -o verifysig -lcrypto verifysig.c
        gcc -o dnswatch dnswatch.c
        gcc -o voucher -lcrypto voucher.c
        gcc -o croen croen.c
        gcc -o pfmon -lpcap pfmon.c
	cc -pthread -o ledindicator  ledindicator.c 
        install -s choparp $MW_BUILDPATH/t1n1fs/usr/local/sbin
        install -s stats.cgi $MW_BUILDPATH/t1n1fs/usr/local/www
        install -s minicron $MW_BUILDPATH/t1n1fs//usr/local/bin
        install -s verifysig $MW_BUILDPATH/t1n1fs/usr/local/bin
        install -s dnswatch $MW_BUILDPATH/t1n1fs/usr/local/bin
        install -s voucher $MW_BUILDPATH/t1n1fs/usr/local/bin
        install -s croen $MW_BUILDPATH/t1n1fs/usr/local/bin
        install -s ledindicator $MW_BUILDPATH/t1n1fs/sbin
        install -s pfmon $MW_BUILDPATH/t1n1fs/sbin
        install ppp-linkup vpn-linkdown vpn-linkup $MW_BUILDPATH/t1n1fs/usr/local/sbin

# select Autoconf version 2.62
#		export AUTOCONF_VERSION=2.62
# net-snmp
        cd $MW_BUILDPATH/tmp
        rm -Rf net-snmp-5.7.3
        tar -zxf $MW_BUILDPATH/freebsd11/build/local-sources/net-snmp-5.7.3.tar.gz
        cd net-snmp-5.7.3
        ./configure --with-sys-contact="contact" --with-sys-location="location" --without-openssl --with-default-snmp-version="2" \
        --enable-ipv6 --disable-set-support --disable-des --disable-privacy --disable-md5 --disable-debugging --enable-mini-agent --disable-testing-code \
        --disable-shared-version --disable-shared '--with-out-transports=TCP Unix TCPIPv6 Callback'  '--with-mib-modules=if-mib host mibII/var_route ucd_snmp utilities/override' \
        --enable-mfd-rewrites --with-defaults
	make
        install -s agent/snmpd $MW_BUILDPATH/t1n1fs/usr/local/sbin

echo "Finished Stage 2"
