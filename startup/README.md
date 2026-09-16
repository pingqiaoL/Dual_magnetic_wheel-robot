# Startup scripts

`etc/init.d/rcS` is compiled into the NuttX `/etc` ROMFS. It currently starts the hardware-independent robot runtime; later versions will add parameter, uORB, SBUS, safety and output modules in dependency order.
