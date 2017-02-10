#sws test shell script

echo -e -n "GET / HTTP/1.0\r\n\r\n" |  nc -u -s 192.168.1.100 10.10.1.100 8080 &
echo -e -n "GET / HTTP/1.0\r\n\r\n" |  nc -u -s 192.168.1.100 10.10.1.100 8080 &
echo -e -n "GET / HTTP/1.0\r\n\r\n" |  nc -u -s 192.168.1.100 10.10.1.100 8080