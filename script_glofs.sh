#!/bin/bash

echo "cd /tmp/glo"
cd /tmp/glo

echo
echo "ls"
ls -lai

echo
echo "./doc"
cd ./doc

echo
echo "ls"
ls -lai

echo
echo "cd .."
cd ..

echo
echo "mkdir ./doc/test"
mkdir ./doc/test

echo
echo "ls ./doc/test"
ls -lai ./doc/test

echo
echo "cd ./doc/test"
cd ./doc/test

echo
echo "ls"
ls -lai

echo
echo "touch touchedFile"
touch touchedFile

echo
echo "ls"
ls -lai

echo
echo "whoami >> touchedFile"
whoami >> touchedFile

echo
echo "ls"
ls -lai

echo
echo "cat touchedFile >> touchedFile"
cat touchedFile >> touchedFile

echo
echo "ls"
ls -lai

echo
echo "cat touchedFile > newfile"
cat touchedFile > newfile

echo
echo "ls"
ls -lai

echo
echo "rm touchedFile"
rm touchedFile

echo
echo "ls"
ls -lai

echo
echo "cp newfile ../copy"
cp newfile ../copy

echo
echo "ls"
ls -lai

echo
echo "ls .."
ls -lai ..

echo
echo "mv newfile ../moved"
mv newfile ../moved

echo
echo "ls"
ls -lai

echo
echo "ls .."
ls -lai ..

echo
echo "cd .."
cd ..

echo
echo "ls"
ls -lai

echo
echo "ln moved linked"
ln moved linked

echo
echo "ls"
ls -lai

echo
echo "mv . /movedDir"
mv . /movedDir

echo
echo "ls"
ls -lai


echo
echo "ls /"
ls -lai