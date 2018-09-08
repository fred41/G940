# Flight System G940 - uinput based driver (linux)  

**User space driver for Flight System G940, based on uinput**  

This is a very simple (but fast and solid) replacement for the G940 kernel driver.  

All 14 analog axes, 3 HAT-Switches and 23 buttons (incl. Mode- and Deadswitch ) are already reported (no FF yet).  

__How to use this driver, overview:__   

-  ensure libusb 1.0 developer package is installed   
-  clone repository from github   
-  build the driver from source   
-  adapt some udev rules to give the driver write access   
to the uinput system and read access to G940 USB device from user space   
-  add your user to the 'input' group, activate the new rules   
-  test the driver with G940 device and a game, or 'evtest' utility   

__Bellow are some commands and example udev rules you could use:__   

>#apt-get install libusb-1.0-0-dev     
$git clone https://github.com/fred41/G940.git    
$cd G940    
$make   


>KERNEL=="uinput", SYMLINK+="input/uinput", GROUP="input"   
SUBSYSTEM=="usb", ATTR{idVendor}=="046d", ATTR{idProduct}=="c287", MODE="0660"  

Don't forget to add your user to group 'input'. Activate the new permissions by a restart.

To test your new G940 driver, run it from terminal (./g940) and use your game to test functionality.   
You can also use 'evtest' utility for testing.  

_Good luck :)_  
