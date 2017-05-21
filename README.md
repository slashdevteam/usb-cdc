# USB CDC

This is a working C++ wrapper for USB stack, aimed at bringing NXP stack to mbed OS.

To make it work put it in mbed OS app and create ```device_specific_descriptors``` struct with device specific configuration and provide it to usb::DeviceClass constructor.
