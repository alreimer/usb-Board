#!/bin/sh
# 1 sda $attr{version} $attr{manufacturer} $attr{product} $attr{idVendor} $attr{idProduct} $attr{serial}


[ $1 == 1 ] && echo "T:  $2 Cls=08(stor.) 
D:  Ver= $3 P:  Vendor=$6 ProdID=$7 
S:  Manufacturer=$4
S:  Product=$5
S:  SerialNumber=$8

" >> /run/devices

[ $1 == 0 ] && sed -i -e "/T:  $2 Cls=08(stor.) /{N;N;N;N;N;N;s/T:  $2 Cls=08(stor.) .*\nD:  Ver= .* P:  Vendor=.* ProdID=.*\nS:  Manufacturer=.*\nS:  Product=.*\nS:  SerialNumber=.*\n\n//;/^$/d;}" /run/devices

chmod 600 /run/devices
