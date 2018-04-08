#!/bin/bash

#the unpack script, by MPrzybylko, to verify the solution integrity
#all questions should be submitted to M.Przybylko <at> mimuw.edu.pl

if [ $# -ne 1 ]
then
  echo "Error: only the name of the archive (w/o .zip)"
  exit 1
fi

#for safe return
p=$(pwd)


echo "Unzipping"
unzip "$1.zip" ||  { echo "Error in unzip"; cd $p; exit 1; }

echo ""

cd $1 ||  { echo "NO $1 dir?"; cd $p; exit 1; }

echo "Building..."
mkdir build

echo ""

cp README* build ||  { echo "NO README?"; cd $p; exit 1; }

echo ""

cd build

cmake .. ||  { echo "Error in cmake"; cd $p; exit 1; }

echo ""

make ||  { echo "Error in make"; cd $p; exit 1; }

echo ""

echo "Built."

echo ""
#check the files
ret=0

declare -a arr=(validator run tester README.pdf)


for name in "${arr[@]}"
do
  if [ ! -e $name ]
  then
    echo "No $name"
    ret=1
  fi
done

echo ""
if [ $ret -gt 0 ]
then
  echo "Error: Missing files"
  exit 1
fi


echo ""
echo "OK: All seems fine!"

cd $p

exit 0