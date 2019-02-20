#!/bin/bash
sudo dd iflag=sync oflag=sync if=spl/smart210-spl.bin of=/dev/sdc seek=1
sudo dd iflag=sync oflag=sync if=u-boot.bin of=/dev/sdc seek=32
sync
echo "done."
