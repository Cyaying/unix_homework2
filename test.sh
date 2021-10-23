#!/bin/bash

echo -e "----------------------------"
echo "-> start compiling ..."
gcc -c myfind.c -o myfind.o
if [ $? -eq 0 ]
then
    echo "-> compile successfully !"
fi

echo "-> start  linking  ..."
gcc error.o pathalloc.o myfind.o -o myfind
if [ $? -eq 0 ]
then
    echo "-> linking successfully !"
fi
echo -e "----------------------------"


echo -e "\n$ TEST FOR QUESTION-1: ./myfind /usr"
./myfind /usr

echo -e "\n$ TEST FOR QUESTION-2: ./myfind /home -comp apue.h"
./myfind /home -comp apue.h

echo -e "\n$ TEST FOR QUESTION-3: ./myfind /home -name apue.h myfind.c"
./myfind /home -name apue.h myfind.c
