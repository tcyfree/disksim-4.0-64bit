#!/bin/bash
rm -rf out-batch.txt

runList="runList.txt" #runList存放准备运行的trace合集

function runTrace() {
  #1.找到运行的excel
  cat $1 | while read line
  do
    echo $line
  done 

  cat $1 | while read line
  do
    saveFile1=${line##*/}
    saveFile2=${saveFile1%%.*}
    
    echo $saveFile2 >> out-batch.txt

    echo $saveFile2
    for i in $(seq 1 20)
    do   
        ../src/syssim $i 0 0 $line >> out-batch.txt
    done

    echo $saveFile2 >> out-batch.txt

    for i in $(seq 1 20)
    do   
        ../src/syssim $i 0 1 $line >> out-batch.txt  
    done 
    
  done 
}
runTrace $runList


 