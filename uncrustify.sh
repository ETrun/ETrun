#!/bin/sh

uncrustify="uncrustify -c uncrustify.cfg --replace --no-backup"

for dir in src; do
  $uncrustify $dir/*/*.c;
  $uncrustify $dir/*/*.h;
done
