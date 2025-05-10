#!/usr/bin/bash 

RC1=1
RC2=1
RC3=1
RC4=1

BASEDIR=$(pwd)

rm -rf ./Release
mkdir ./Release

echo --------------------------------------------
echo Building zsd
echo --------------------------------------------
cd zsd
make -f zsd_release clean all
RC1=$?
echo RC1=$RC1
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Building zss
echo --------------------------------------------
cd zss
make -f zss_release clean all
RC2=$?
echo RC2=$RC2
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Building zsc
echo --------------------------------------------
cd zsc
mvn clean package
RC3=$?
echo RC3=$RC3
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Building zs-client
echo --------------------------------------------
cd zs-client
mvn clean package
RC4=$?
echo RC4=$RC4
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Copying binaries
echo --------------------------------------------
echo cp ./zsd/Build/Release/zsd ./Release
cp ./zsd/Build/Release/zsd ./Release
echo cp ./zss/Build/Release/zss ./Release
cp ./zss/Build/Release/zss ./Release

echo cp ./zsc/target/zsc-*.jar ./Release
cp ./zsc/target/zsc-*.jar ./Release
echo cp ./zsc/target/zs-client*.jar ./Release
cp ./zs-client/target/zs-client*.jar ./Release

echo ""
echo --------------------------------------------
echo Binaries
echo --------------------------------------------
ls -la ./Release

echo ""
echo RC1=$RC1
echo RC2=$RC2
echo RC3=$RC3
echo RC4=$RC4

