#!/bin/bash

#make smart210_defconfig

make uImage -j4

echo "copy to ../bin/smart210"
cp arch/arm/boot/uImage ~/bin/smart210
