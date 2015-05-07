bfgminer -S ICA:\\.\COM6 -S ICA:\\.\COM10 -S ICA:\\.\COM11 -S ICA:\\.\COM12 -S ICA:\\.\COM3 -S ICA:\\.\COM47 ^
-o http://p2pool.org:9332 -O 1GG4mvU4E7U8yHEEXQ17QsP5xXM2ZYBBDp:useless ^
--set-device ICA:baud=115200 ^
--set-device ICA:reopen=timeout ^
--set-device ICA:work_division=1 ^
--set-device ICA:fpga_count=1 ^
--set-device ICA:probe_timeout=100 ^
--set-device ICA:timing=0.22 ^
--api-listen 2>log ^
-D -T