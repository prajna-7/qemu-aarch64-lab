# Prajna QEMU Lab — U-Boot + Linux + Custom Device Tree Bring-Up 


---
## Summary
This repository is a *complete, demonstrable project* showing how to build U-Boot, the Linux kernel (ARM64), a minimal initramfs or ext4 rootfs, add custom device-tree nodes at boot, and run it all under QEMU `virt` (aarch64).

**This package is prepared for presentation / repro on Ubuntu hosts.**  
Large binary artifacts (kernel Image, u-boot.bin, rootfs.ext4, initramfs.cpio.gz) are **excluded** due to size — placeholders and instructions are included so anyone can reproduce the build locally using the commands below.

The README shows all build commands used during development so you can reproduce the images exactly.

---
## Quick structure (important files)
```
Prajna-QEMU-Lab/
├── README.md
├── LICENSE
├── Makefile
├── images/
│   ├── boot-disk.cmd
│   ├── boot-disk.scr
│   ├── boot.cmd
│   ├── boot.scr
│   ├── dt-proof.txt
│   └── virt-custom.dtb     (generated at runtime; placeholder)
├── modules/lab_sensor/
│   ├── lab_sensor.c
│   ├── Makefile
│   └── README.md
├── scripts/
│   ├── prepare-env.sh
│   ├── build-uboot.sh
│   ├── build-kernel.sh
│   ├── build-busybox-rootfs.sh
│   ├── mk-uboot-script.sh
│   └── run-qemu.sh
├── share/demo-logs/
│   ├── qemu-initramfs.log
│   ├── qemu-ext4.log
│   └── module-probe.log
└── tools/Makefile.tools
```

---
## Repro (detailed commands)
> Run these on an Ubuntu (24.04) host with development packages installed (see `scripts/prepare-env.sh`). Commands assume you are in the repo root `Prajna-QEMU-Lab/` and `$LAB=$PWD`.

### 0) Prepare host (one-time)
```bash
# install build deps (example list)
sudo apt update
sudo apt install -y build-essential git bc bison flex libssl-dev     libncurses-dev device-tree-compiler qemu-system-arm qemu-system-aarch64     gcc-aarch64-linux-gnu g++-aarch64-linux-gnu u-boot-tools     debootstrap rsync gcc-arm-none-eabi python3-pip

# (optional) increase ulimit for parallel builds
ulimit -n 4096
```

### 1) Clone sources (mainline kernels and U-Boot)
```bash
# example layout used in the project
export LAB=$PWD
mkdir -p $LAB/src $LAB/build $LAB/images $LAB/modules $LAB/tools $LAB/share
cd $LAB/src
git clone --depth=1 https://github.com/u-boot/u-boot.git u-boot
git clone --depth=1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux
git clone --depth=1 https://git.busybox.net/busybox busybox
# Note: you may want specific tags/commits; the build logs in share/ document the versions used.
```

### 2) Build BusyBox + minimal rootfs (initramfs or ext4)
```bash
cd $LAB/src/busybox
make distclean
make defconfig
# Optional: tweak .config (make menuconfig)
make -j$(nproc)
make CONFIG_PREFIX=$LAB/build/busybox/_install install

# Create minimal ext4 rootfs (example)
cd $LAB/build
mkdir -p rootfs-disk/{bin,sbin,proc,sys,dev,etc,root,tmp,boot,usr}
rsync -a $LAB/build/busybox/_install/ rootfs-disk/
# Create inittab and rcS as described in the README in scripts/
sudo mke2fs -F -t ext4 -L rootfs -d rootfs-disk $LAB/images/rootfs.ext4 256M
# OR create initramfs.cpio.gz if you prefer initramfs boot (scripts provided)
```

### 3) Build Linux kernel (ARM64)
```bash
cd $LAB/src/linux
# configure for qemu virt
make ARCH=arm64 defconfig
# optional: make menuconfig to enable modules and DT options
# enable CONFIG_MODULES and drivers you need
make -j$(nproc) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image
# copy image for QEMU usage
cp arch/arm64/boot/Image $LAB/images/Image
# extract or produce DTB (the project used QEMU dumpdtb or the arch DTs)
dtc -I dtb -O dts -o $LAB/images/qemu-virt.dts $LAB/images/qemu-virt.dtb  # example
```

### 4) Build U-Boot for QEMU `virt` (AArch64)
```bash
cd $LAB/src/u-boot
# example build for qemuvirt/aarch64 (adjust defconfig name to your U-Boot)
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- qemu_arm64_defconfig
make -j$(nproc) ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
# copy the produced u-boot.bin (or u-boot.elf) to images/
cp u-boot.bin $LAB/images/u-boot.bin
```

### 5) Build kernel module (lab_sensor)
```bash
# in repo, we've included an out-of-tree driver under modules/lab_sensor/
export KDIR=$LAB/src/linux
make -C $KDIR M=$LAB/modules/lab_sensor ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
# copy the resulting lab_sensor.ko to the ext4 root or insmod via init
```

### 6) Boot QEMU — two common modes used in development
**A) Boot kernel directly (fast test)**
```bash
qemu-system-aarch64 -machine virt -cpu cortex-a53 -smp 2 -m 1024   -nographic -kernel $LAB/images/Image -initrd $LAB/images/initramfs.cpio.gz   -append "console=ttyAMA0 earlycon=pl011,0x09000000 panic=-1"
```

**B) Boot via U-Boot (scripted) — recommended for DT edits**  
U-Boot `boot-disk.cmd` included demonstrates loading the kernel from fw_cfg or disk,
creating a custom node (`/custom-node`) and a `/lab-sensor`, then booti-ing kernel with
a modified FDT that contains:
- `/custom-node/message = "hello-from-disk-boot"`
- `/lab-sensor/label = "virtual-temp0"`
- `/lab-sensor/temp-millic = 42000` (big-endian)
- `/lab-sensor/id-bytes = de ad be ef ca fe ba be`

To run via U-Boot script (when you have `u-boot.bin` in images/):
```bash
qemu-system-aarch64 -machine virt -cpu cortex-a53 -smp 2 -m 1024 -nographic   -bios $LAB/images/u-boot.bin   -drive file=$LAB/images/rootfs.ext4,if=virtio,format=raw   -device loader,file=$LAB/images/boot-disk.scr,addr=0x42000000
```

### 7) Verify runtime device-tree and module
Inside the VM (serial console):
```sh
# show the injected node
ls /proc/device-tree/custom-node
cat /proc/device-tree/custom-node/message  # shows 'hello-from-disk-boot' (NUL-terminated)
# check sensor node
tr -d '\0' < /proc/device-tree/lab-sensor/label; echo  # virtual-temp0
# read big-endian temp
hexdump -v -e '1/4 "0x%08x\n"' /proc/device-tree/lab-sensor/temp-millic
# insert module if not auto-loaded:
insmod /root/lab_sensor.ko
dmesg | tail -n 30  # shows probe messages
```

---
## Files marked *placeholder* (you should replace these with real artifacts):
- `images/Image` (kernel Image) — **NOT included**.
- `images/u-boot.bin` — **NOT included**.
- `images/initramfs.cpio.gz` and `images/rootfs.ext4` — **NOT included**.

Place them under `images/` with the same names to use the included scripts.

