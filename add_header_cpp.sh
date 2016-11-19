#!/bin/bash
FILES=$1
echo "got dir ::$1"
for f in $FILES/*.cpp
do
  echo "Processing $f file..."
 # sed -i '1s;^;#include <stdio.h> \n;' $f
  sed -i '1s;^;#include <stdlib.h> \n #include <string.h> \n;' $f
 # is_cpp=`cat $f | grep -c -m1 cout`
 # #echo `cat $f`
 # echo "is cpp?::$is_cpp"
 # if [ "$is_cpp" == "1" ]; then
 #     echo "Yes CPP"
 #     #echo -e "#include <iostream> \n using namespace std;\n$(cat $f)" > $f
 #     sed -i '1s;^;#include <iostream> \n using namespace std\;\n;' $f
 #     mv "$f" "$FILES/`basename "$f" .c`.cpp"
 # fi

  # take action on each file. $f store current file name
done

