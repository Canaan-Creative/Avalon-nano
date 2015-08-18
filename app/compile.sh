#!/bin/bash

#
# Compile
#

[[ -f ./compiler.jar ]] || (wget http://dl.google.com/closure-compiler/compiler-latest.zip && unzip compiler-latest.zip compiler.jar && rm compiler-latest.zip)

[[ -d compile ]] || mkdir compile
cd compile
cp ../sha256.js ./
cp ../index.html ./
cp -r ../images ./
cp -r ../style ./
cp -r ../assets ./
cp -r ../js ./

< ../manifest.json jq -c '.app.background.scripts=["sha256.js","avalon.js","background.js"]' > ./manifest.json

cat ../utils.js ../pool.js ../driver.js > avalon.js
java -jar ../compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js avalon.js --js_output_file avalon.min.js
mv avalon.min.js avalon.js

java -jar ../compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js ../background.js --js_output_file background.js

cp ../thread.js ./
sed -i -e "s/^importScripts.*$/importScripts('avalon.js','sha256.js');/g" thread.js
java -jar ../compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js thread.js --js_output_file thread.min.js
mv thread.min.js thread.js

java -jar ../compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js js/custom.js --js_output_file js/custom.min.js
mv js/custom.min.js js/custom.js


#
# Pack
#
# TO BEGIN, INSTALL swig & crxmake:
# apt-get install swig
# pip2 install crxmake

cd ..
crxmake -o app.crx compile
mv app.crx Avalon_miner_v`< ./manifest.json jq -r -M '.version'`.`git ls-tree master . | git mktree | cut -c-8`.crx


#
# Clean
#
rm -rf compile
