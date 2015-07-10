#!/bin/bash

cd ..

#Client begins interaction and uses up its allotment and renews
./ngp start && echo "Daemon starting."
sleep 1
./ngp server "send asdf 123 request"
./ngp server "receive asdf 123 propose 5 0 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 propose 4 0 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 begin"
./ngp server "receive asdf 123 count_packets 240"
./ngp stop

#Client begins interaction and uses up its allotment and does not renew
./ngp start && echo "Daemon starting."
sleep 1
./ngp server "send asdf 123 request"
./ngp server "receive asdf 123 propose 5 5 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 propose 4 5 "$(date --date "now +5 minute" +%s)
./ngp server "receive asdf 123 begin"
./ngp server "receive asdf 123 count_packets 5"
./ngp server "send asdf 123 stop"
./ngp server "receive asdf 123 count_packets 500"
./ngp stop

#Server receives interaction and shuts connection after the contract times out
./ngp start && echo "Daemon starting."
sleep 1
./ngp server "receive asdf 123 request"
./ngp server "receive asdf 123 reject 4 5 "$(date --date "now +2 second" +%s)
./ngp server "receive asdf 123 accept"
./ngp server "receive asdf 123 payment 100"
sleep 3
./ngp server "receive asdf 123 count_packets 1"
./ngp stop

#Server receives interaction and shuts connection after the client uses up their allotment
./ngp start && echo "Daemon starting."
sleep 1
./ngp server "receive asdf 123 request"
./ngp server "receive asdf 123 reject 4 5 "$(date --date "now +2 second" +%s)
./ngp server "receive asdf 123 accept"
./ngp server "receive asdf 123 payment 100"
./ngp server "receive asdf 123 count_packets 10"
./ngp server "receive asdf 123 count_packets 23"
./ngp stop

