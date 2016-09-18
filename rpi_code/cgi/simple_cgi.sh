#!/bin/bash

echo "Content-type: text/html"
echo ""


echo "QUERY_STRING $QUERY_STRING"
echo "<br>"
echo "REMOTE_ADDR $REMOTE_ADDR"

ARGS_STR=`echo $QUERY_STRING | tr "+" " "`

echo "/home/pi/spidev_led_matrix $ARGS_STR" &>> /tmp/spidev_led_matrix_web_access.log &
/home/pi/spidev_led_matrix $ARGS_STR &>> /tmp/spidev_led_matrix_web_access.log &

echo "<pre>"
ls -l /home/pi
echo "</pre>"



