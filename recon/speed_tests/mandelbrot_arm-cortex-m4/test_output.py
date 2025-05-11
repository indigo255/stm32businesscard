#!/bin/bash
make clean && 
make && 
./write.sh && 
doas stty -F /dev/ttyACM0 115200 &&
cat /dev/ttyACM0 > out.csv
