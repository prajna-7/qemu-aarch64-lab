# Boot-disk command used by U-Boot to inject DT nodes and boot ext4 root
setenv kernel_addr_r 0x40400000
setenv ramdisk_addr_r 0x44000000
qfw load ${kernel_addr_r} ${ramdisk_addr_r}

# MACHINE FDT (bdinfo shows fdt_blob)
setenv fdt_machine 0x7e559dc0
fdt addr ${fdt_machine}
fdt resize

# custom nodes
fdt mknode / custom-node || true
fdt set /custom-node compatible "lab,custom-node"
fdt set /custom-node status "okay"
fdt set /custom-node message "hello-from-disk-boot"

fdt mknode / lab-sensor || true
fdt set /lab-sensor compatible "lab,temp-sensor"
fdt set /lab-sensor status "okay"
fdt set /lab-sensor label "virtual-temp0"
fdt rm /lab-sensor temp-millic || true
fdt set /lab-sensor temp-millic [00 00 a4 10]
fdt set /lab-sensor id-bytes [de ad be ef ca fe ba be]

setenv bootargs 'console=ttyAMA0 earlycon=pl011,0x09000000 panic=-1 root=/dev/vda rw rootwait rootfstype=ext4'
booti ${kernel_addr_r} - ${fdt_machine}
