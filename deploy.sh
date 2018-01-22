#!/usr/bin/env bash

echo "preparing files and folders..."
mkdir kk385830 >/dev/null 2>&1
rm kk385830.zip >/dev/null 2>&1

echo "exporting pdf..."
markdown-pdf README.md -o kk385830/README.pdf >/dev/null 2>&1

echo "copying files..."
cp -r src kk385830/
cp -r test kk385830/
cp -r large_examples kk385830/
cp -r original_examples kk385830/
cp tester.c kk385830/
cp validator.c kk385830/
cp run.c kk385830/
cp CMakeLists.txt kk385830/

echo "zipping..."
zip -r kk385830.zip kk385830 >/dev/null 2>&1

echo "cleaning files..."
rm -r kk385830 >/dev/null 2>&1

echo "done"
