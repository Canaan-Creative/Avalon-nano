#!/bin/bash
[[ -f ./compiler.jar ]] || (wget http://dl.google.com/closure-compiler/compiler-latest.zip && unzip compiler-latest.zip compiler.jar && rm compiler-latest.zip)

git checkout origin/master images
git checkout origin/master style
git checkout origin/master utils.js
git checkout origin/master pool.js
git checkout origin/master driver.js
cat utils.js pool.js driver.js > avalon.js
java -jar ./compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js avalon.js --js_output_file avalon.min.js
mv avalon.min.js avalon.js
git rm -f utils.js pool.js driver.js
rm -f utils.js pool.js driver.js

git checkout origin/master background.js
java -jar ./compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js background.js --js_output_file background.min.js
git reset HEAD background.js
mv background.min.js background.js

git checkout origin/master thread.js
sed -i -e "s/^importScripts.*$/importScripts('avalon.js','sha256.js');/g" thread.js
java -jar ./compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js thread.js --js_output_file thread.min.js
git reset HEAD thread.js
mv thread.min.js thread.js

git checkout origin/master js/custom.js
java -jar ./compiler.jar --language_in=ECMASCRIPT6 --language_out=ECMASCRIPT5 --js js/custom.js --js_output_file js/custom.min.js
git reset HEAD js/custom.js
mv js/custom.min.js js/custom.js
