# usb2sniffer-cli

lcsniff-cli software for [USB2Sniffer hardware](http://blog.lambdaconcept.com/doku.php?id=products:usb_sniffer) ([buy](http://shop.lambdaconcept.com/home/35-usb2-sniffer.html)). It currently spits out pcap but also usb files that you can load in [usb2sniffer-qt](https://github.com/lambdaconcept/usb2sniffer-qt).

Linux only currently + requires [a kernel driver](https://github.com/lambdaconcept/ft60x_driver).

## Build

No external libs required on Linux.

```
make
```

## HowTo

```
./lcsniff fs /dev/ft60x0 test.pcap
./lcsniff fs /dev/ft60x0 test.usb
```
