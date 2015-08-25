# Chrome App for Avalon Nano

## Compile and Launch from CRX

* Compile
	```
	sudo apt-get install jq python-dev swig
	sudo pip2 install crxmake
	./compile
	```

* Bring up the apps and extensions management page by clicking the settings icon and choosing Tools > Extensions.

* Drag and drop the crx file to the chrome/chromium page.

* Click OK to install.


## Launch from Folder
Ref: https://developer.chrome.com/apps/first_app#load

* Bring up the apps and extensions management page by clicking the settings icon and choosing Tools > Extensions.

* Make sure the Developer mode checkbox has been selected.

* Click the Load unpacked extension button, navigate to your app's folder and click OK.

## UDEV Rule for Linux
`KERNEL=="hidraw*", SUBSYSTEM=="hidraw", ATTRS{idVendor}=="29f1", MODE="0664", GROUP="plugdev"`


## Links
* https://github.com/progranism/Bitcoin-JavaScript-Miner
* https://github.com/bitcoinjs/bitcoinjs-lib
* https://github.com/derjanb/hamiyoca

## Third Party
* [jsSHA](https://github.com/Caligatio/jsSHA)(BSD-licensed) for SHA-256 algorithm
