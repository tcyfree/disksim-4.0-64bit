#!/bin/bash
echo $1
for i in $(seq 1 10)
do   
    ../src/syssim $i 0 0 $1
done
echo $1
for i in $(seq 1 10)
do   
    ../src/syssim $i 0 1 $1
done 


 