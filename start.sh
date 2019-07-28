#/bin/sh

./train victim 0x400760 0x400877 &

pid=$(ps -ef | grep "train" | grep -v grep | awk '{print $2}')

./attack victim 0x6310a0 32

kill $pid

