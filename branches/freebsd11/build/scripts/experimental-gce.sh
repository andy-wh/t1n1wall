#!/usr/local/bin/bash


if [ -z "$MW_BUILDPATH" -o ! -d "$MW_BUILDPATH" ]; then
        echo "\$MW_BUILDPATH is not set"
        exit 1
fi


VERSION=`cat $MW_BUILDPATH/t1n1fs/etc/version`
if [ $MW_ARCH = "amd64" ]; then
        VERSION=$VERSION.$MW_ARCH
fi

PLATFORM=generic-pc-serial

cd  $MW_BUILDPATH/tmp/

mkdir gce_build
cd gce_build

echo building $PLATFORM-$VERSION.gce.tar.gz

cp $MW_BUILDPATH/images/$PLATFORM-$VERSION.img disk.raw.gz
gunzip disk.raw.gz
gtar -Sczf $MW_BUILDPATH/images/$PLATFORM-$VERSION.gce.tar.gz disk.raw


cd $MW_BUILDPATH/tmp/
rm -rf gce_build

echo "Finished GCE image Build"

