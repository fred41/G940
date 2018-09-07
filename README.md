# Flight System G940 uinput based driver (linux)  
  
A user space driver for Flight System G940, based on uinput (linux only)  
  
This is a very simple (but fast and solid) replacement for the G940 kernel driver.  
  
All axes, buttons and switches are already reported correctly.  
Force-Feedback is not implemented currently.  
  
To use this driver:    
  
ensure 'libusb-1.0-dev' packet is installed on your system  
clone this repo 'git clone https://github.com/fred41/G940.git'   
cd G940  
make  
  
There should be a file 'g940' now (the driver executable).  
  
You have to add (most likely) a new udev rule, to give the driver access  
to the G940 USB device from user space:  
  
SUBSYSTEM=="usb", ATTR{idVendor}=="046d", ATTR{idProduct}=="c287", MODE="0666"  
  
Activate the new rule by a restart.  
  
Plug in your G940, run the driver from terminal (./g940) and 'evtest' or your game   
to test functionality.  
  
Good luck :)  
