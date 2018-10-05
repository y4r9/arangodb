#!/bin/bash
echo "open sockets: "
F="/tmp/$$_netstat.log"
netstat -n |grep ^tcp > $F
C=$(wc -l < "$F")
echo $C
if test "$C" -gt 1024; then
     LF="/tmp/$$_lsof.log"
     lsof -n -iTCP > "$LF"
     for sockets in $(grep ^tcp $F | sed -e "s;  *; ;g"  |cut -d ' '  -f 5 |sort -u ) ; do 
	SC=$(grep "$sockets" $F |wc -l)
	if test "$SC" -gt "16"; then
		echo "$sockets - $SC"
		grep "$sockets" "$LF"
	fi
   done

fi
rm "$F"

