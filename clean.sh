#!/usr/bin/bash 

RC1=1
RC2=1
RC3=1
RC4=1
RC5=1
RC6=1

BASEDIR=$(pwd)

rm -rf ./Release

echo --------------------------------------------
echo Cleaning zsd release
echo --------------------------------------------
cd zsd
make -f zsd_release clean
RC1=$?
echo RC1=$RC1
cd "$BASEDIR"

echo --------------------------------------------
echo Cleaning zsd eclipse
echo --------------------------------------------
cd zsd/Debug
make clean
RC2=$?
echo RC2=$RC2
cd "$BASEDIR"

echo --------------------------------------------
echo Cleaning zss release
echo --------------------------------------------
cd zss
make -f zss_release clean
RC3=$?
echo RC3=$RC3
cd "$BASEDIR"

echo --------------------------------------------
echo Cleaning zss eclipse
echo --------------------------------------------
cd zss/Debug
make clean
RC4=$?
echo RC4=$RC4
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Cleaning zsc
echo --------------------------------------------
cd zsc
mvn clean
RC5=$?
echo RC5=$RC5
cd "$BASEDIR"

echo ""
echo --------------------------------------------
echo Cleaning zs-client
echo --------------------------------------------
cd zs-client
mvn clean
RC6=$?
echo RC6=$RC6
cd "$BASEDIR"

echo ""
echo RC1=$RC1
echo RC2=$RC2
echo RC3=$RC3
echo RC4=$RC4
echo RC5=$RC5
echo RC6=$RC6

