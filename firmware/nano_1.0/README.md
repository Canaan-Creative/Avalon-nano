Avalon miner
==========

Directory structure
===================
* `firmware`: Avalon nano firmware
* `tools`: Tool for developers
* `testbench`: Module test for Avalon nano

Quick start
==========
* For windows users:

http://downloads.canaan-creative.com/software/avalon_nano/doc/

* For Linux users:

./bfgminer \
       -S ICA:/dev/ttyACM0 \
       -o stratum+tcp://stratum.ozco.in:80 -O user:pass \
       --set-device ICA:baud=115200 \
       --set-device ICA:reopen=timeout \
       --set-device ICA:work_division=1 \
       --set-device ICA:fpga_count=1 \
       --set-device ICA:probe_timeout=100 \
       --set-device ICA:timing=0.22 \
       --api-listen \
       2>log
       
or

./cgminer \
       -o stratum+tcp://stratum.ozco.in:80 -O user:pass \
       --icarus-options 115200:1:1 \
       --icarus-timing 0.22 \
       --api-listen \
       2>log
       
IDE Support
=============
* LPCXpresso 6 or above (Free Edition)
* Keil 5.1

Miner Support
=======
* Cgminer (https://github.com/Canaan-Creative/cgminer/tree/avalon-usb)
* bfgminer (https://github.com/luke-jr/bfgminer)

OS Support
=======
* Linux
* Windows xp, Windows 7, Window 8

Discussion
==========
* IRC: #avalon @freenode.net
* [![Gitter chat](https://badges.gitter.im/Canaan-Creative/Avalon-nano.png)](https://gitter.im/Canaan-Creative/Avalon-nano)
* Mailing list: http://lists.canaan-creative.com/
* Documents/Downloads: https://en.bitcoin.it/wiki/Avalon_nano

License
=======
This is free and unencumbered public domain software. For more information,
see http://unlicense.org/ or the accompanying UNLICENSE file.

Files under verilog/superkdf9/ have their own license (Lattice Semi-
conductor Corporation Open Source License Agreement).

Some files may have their own license disclaim by the author.

Links
=====
* http://www.element14.com/community/servlet/JiveServlet/downloadBody/52460-102-1-266572/NXP.Application_Note_3.pdf
* http://www.nxp.com/redirect/usb.org/developers/devclass_docs/DFU_1.1.pdf
