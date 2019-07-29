#!/bin/sh

./train victim 0x400760 0x400877 &

pid1=$(ps -ef | grep "train" | grep -v grep | awk '{print $2}')

sleep 3

./attack victim 0x6310a0 16 300
#gdb attack

pid2=$(ps -ef | grep "victim" | grep -v grep | awk '{print $2}')

sleep 1

for i in $pid2
do
    kill $i
done

kill $pid1

