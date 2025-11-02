# ARM64 QEMU Lab – U-Boot + Linux Kernel + Device Tree Bring-Up

This project demonstrates a complete ARM64 embedded Linux bring-up workflow using QEMU, including:

**U-Boot → Device Tree → Linux Kernel → Root Filesystem → Kernel Module**

It simulates early silicon bring-up and Board Support Package (BSP) development — similar to real SoC platform engineering workflows.

> Developed & tested on Ubuntu 24.04  
> NOTE: Kernel and U-Boot binaries are not committed due to size.  
> Build steps provided to reproduce exactly.

---

## Features

- Build & configure **U-Boot** for QEMU ARM64 (`virt`)
- Build **Linux kernel (ARM64)** with modules + debug config
- Create **BusyBox initramfs** + **ext4 root filesystem**
- Inject custom Device Tree nodes at boot using U-Boot `fdt` commands
- Boot Linux & verify DT nodes under `/proc/device-tree`
- Implement **kernel module (lab_sensor)** reading DT properties
- Debug boot issues (rootfs, symbols, DT formatting)

---

## Skills Demonstrated

- U-Boot internals, device tree commands, boot automation
- Linux kernel build, configuration, module development
- Device Tree structure + runtime editing
- QEMU ARM64 virtual platform bring-up
- Kernel debug techniques: `dmesg`, earlycon, printk
- Cross-compiling toolchains & embedded filesystem creation

---

## Repository Structure

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
│   └── virt-custom.dtb
├── modules/
│   └── lab_sensor/
│       ├── lab_sensor.c
│       ├── Makefile
│       └── README.md
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
└── tools/
    └── Makefile.tools
```

> Kernel image, initramfs, U-Boot binary, rootfs ext4 MUST be placed into `images/` after building.

---

## Environment Setup (Ubuntu 24.04)

```bash
sudo apt update
sudo apt install -y build-essential git bc bison flex libssl-dev \
  libncurses-dev device-tree-compiler qemu-system-arm qemu-system-aarch64 \
  gcc-aarch64-linux-gnu g++-aarch64-linux-gnu u-boot-tools \
  debootstrap rsync gcc-arm-none-eabi python3-pip
ulimit -n 4096
```

---

## Fetch Sources

```bash
export LAB=$PWD
mkdir -p $LAB/src $LAB/build $LAB/images $LAB/modules $LAB/share
cd $LAB/src

git clone --depth=1 https://github.com/u-boot/u-boot.git u-boot
git clone --depth=1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git linux
git clone --depth=1 https://git.busybox.net/busybox busybox
```

---

## Build BusyBox + RootFS

```bash
cd $LAB/src/busybox
make distclean
make defconfig
make -j$(nproc)
make CONFIG_PREFIX=$LAB/build/busybox/_install install
```

```bash
cd $LAB/build
mkdir -p rootfs-disk/{bin,sbin,proc,sys,dev,etc,root,tmp,boot,usr}
rsync -a $LAB/build/busybox/_install/ rootfs-disk/
sudo mke2fs -F -t ext4 -L rootfs -d rootfs-disk $LAB/images/rootfs.ext4 256M
```

---

## Build Linux Kernel

```bash
cd $LAB/src/linux
make ARCH=arm64 defconfig
make -j$(nproc) ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- Image
cp arch/arm64/boot/Image $LAB/images/Image
```

---

## Build U-Boot

```bash
cd $LAB/src/u-boot
make ARCH=arm CROSS_COMPILE=aarch64-linux-gnu- qemu_arm64_defconfig
make -j$(nproc) ARCH=arm CROSS_COMPILE=aarch64-linux-gnu-
cp u-boot.bin $LAB/images/u-boot.bin
```

---

## Build Kernel Module

```bash
export KDIR=$LAB/src/linux
make -C $KDIR M=$LAB/modules/lab_sensor ARCH=arm64 CROSS_COMPILE=aarch64-linux-gnu- modules
```

---

## Boot in QEMU

### Direct kernel boot (fast)

```bash
qemu-system-aarch64 -machine virt -cpu cortex-a53 -smp 2 -m 1024 -nographic \
  -kernel $LAB/images/Image -initrd $LAB/images/initramfs.cpio.gz \
  -append "console=ttyAMA0 earlycon=pl011,0x09000000 panic=-1"
```

### Boot via U-Boot (with DT injection)

```bash
qemu-system-aarch64 -machine virt -cpu cortex-a53 -smp 2 -m 1024 -nographic \
  -bios $LAB/images/u-boot.bin \
  -drive file=$LAB/images/rootfs.ext4,if=virtio,format=raw \
  -device loader,file=$LAB/images/boot-disk.scr,addr=0x42000000
```

---

## Verify Device Tree Nodes + Kernel Module

```sh
ls /proc/device-tree/custom-node
cat /proc/device-tree/custom-node/message

tr -d '\0' < /proc/device-tree/lab-sensor/label
hexdump -v -e '1/4 "0x%08x\n"' /proc/device-tree/lab-sensor/temp-millic

insmod /root/lab_sensor.ko
dmesg | tail -n 20
```

---

## Required Files to Place in `images/`

| File | Purpose |
|------|--------|
| `Image` | Linux kernel |
| `u-boot.bin` | Bootloader |
| `rootfs.ext4` | Root filesystem |
| `initramfs.cpio.gz` | Initramfs |

---

## Summary

This lab demonstrates:
- Full boot chain on ARM64 virtual hardware
- Bootloader bring-up, kernel build, DT modification
- Custom kernel driver development
- Embedded debugging and SoC bring-up methodology

Designed to mirror real embedded Linux + kernel platform development workflows.
