#!/bin/sh

#Setup - Enable only one GPRMC Message
echo "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29\r\n" > /dev/ttyS3
read -r response < /dev/ttyS3
echo $response

while read -r data < /dev/ttyS3; do
  sleep 3
  var=$(echo $data | awk '{ split($0,a,","); print a[3]}')
  if [ "$var" == "V" ]; then
#	echo "Gps-Status 'V': GPS data not valid !!"
  else
       # echo "GPS valid ... Saving into File!"
	echo date  
        echo $data > /sdcard/gps/log.txt
	
  fi
done

