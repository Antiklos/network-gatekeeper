#!/bin/bash

cd ..

./ngp start && echo "Daemon starting."
sleep 1
./ngp server "send asdf 123"
./ngp server "receive asdf 123 propose 5 500 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 propose 4 500 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 begin"
./ngp stop

./ngp start && echo "Daemon starting."
sleep 1
./ngp server "receive asdf 123 request"
./ngp server "receive asdf 123 reject 4 500 "$(date --date "now +1 minute" +%s)
./ngp server "receive asdf 123 accept"
./ngp stop

