#!/bin/bash

for f in FMODDOC.TXT  it.txt  mod.txt  s3m.txt  xm_errata.txt  xm.txt
do
    iconv -f 437 -t UTF-8 $f > ../$f
done
