#!/bin/bash
BASE_DIR=$1
echo "got dir ::$1"
for sub_dir in $BASE_DIR/*
do
  if [ -d "$sub_dir" ]
  then
    echo "Found dir $sub_dir"
    for c_files in $sub_dir/*.c
    do
        echo "c file is $c_files"
      is_cpp=`cat $c_files | grep -c -m1 cin`
      #echo "is cpp?::$is_cpp"
      if [ "$is_cpp" == "1" ]; then
          echo "Yes CPP $c_files"
          sed -i '1s;^;#include <iostream> \n using namespace std\;\n;' $c_files
          mv "$c_files" "$BASE_DIR/`basename "$sub_dir"`__`basename "$c_files" .c`.cpp"
          #exit
      else 
          echo "No  CPP $c_files"
          sed -i '1s;^;#include <stdlib.h> \n #include <stdio.h> \n #include <string.h>\n;' $c_files
          mv "$c_files" "$BASE_DIR/`basename "$sub_dir"`__`basename "$c_files" .c`.c"
          #exit
      fi
        #mv "$c_files" "$BASE_DIR/`basename "$sub_dir"`__`basename "$c_files"`"
    done 
  fi
  #echo "Processing $f file..."
  #is_cpp=`cat $f | grep -c -m1 cin`
  ##echo "is cpp?::$is_cpp"
  #if [ "$is_cpp" == "1" ]; then
  #    echo "Yes CPP"
  #    echo -e "#include <iostream> \n using namespace std;\n$(cat $f)" > $f
  #    sed -i '1s;^;#include <iostream> \n using namespace std\;\n;' $f
  #    mv "$f" "$FILES/`basename "$f" .c`.cpp"
  #exit
  #fi

  # take action on each file. $f store current file name
done

