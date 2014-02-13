#!/bin/sh

#mount -o nolock,rsize=1024 192.168.1.2:/home/rbrtbrehm/performand.k70.2/release/linux-cortexm-1.11.0/projects/performand /mnt
mount -o nolock,rsize=1024 192.168.1.2:/home/rasmus/performand.k70.2/release/linux-cortexm-1.11.0/projects/performand /mnt

echo "Set ttyS1 serial port to 9600 Baud"
stty -F /dev/ttyS1 9600 

echo "Set ttyS3 serial port to 115200 Baud"
stty -F /dev/ttyS3 115200 

echo "Set ttyS4 serial port to 115200 Baud"
stty -F /dev/ttyS4 115200 

#echo "Preparing cgi appliaction."
#cp read_humtemp
#/httpd/html

#echo "Starting HTML server."
#httpd -h /httpd/html

#echo "Starting battery log."
#./monitor_bat.sh &

# Update runtime counter (block until finished updating)
./performand/apps/inc_runtime_cnt

echo "Starting GPS daemon ..."
#./performand/apps/gps_daemon &
#./mnt/apps/daemons/gps_daemon &

echo "Starting IMU daemon ..."
#./performand/apps/imu_daemon -b 0 -y 1 &
#./mnt/apps/mpu*/imu -b 0 -y 1 &

#echo "Starting Bluetooth daemon ..."
#./performand/apps/bluetooth &

echo "Starting Wifi AP and TCP Service daemon ..."
#./mnt/apps/wifi/main
./performand/apps/wifi_ap_init

echo " "
echo " "
echo "****************************************************"
echo "W E L C O M E   T O   P E R F O R M A N D"
echo " "
echo "DataBoy (v2.0) Generic Data Controller ..."
echo " "
echo "... and Sailing Performance."
echo " "
echo "by Mads Clausen Institute, 2014"
echo " "
echo " "
echo "For questions please contact:"
echo "brehm@mci.sdu.dk - 0045 6550 1612"
echo "rasko@mci.sdu.dk - 0045 6550 1636"
echo " "
echo " "
echo " financed by Interreg K.E.R.N "
echo "****************************************************"
echo " "
echo " "

echo "Starting Data Aggregation and TCP stream ..."
#./performand/apps/stream_data &
#./mnt/apps/wifi/w_tcp_str &

