.PHONY: help build-all run-qemu package
help:
	@echo "Targets:"
	@echo "  make build-all    # Run scripts to prepare environment (no large binaries)"
	@echo "  make run-qemu     # Launch QEMU (requires images/* binaries present)"
	@echo "  make package      # Create zip (already done)"
build-all:
	@bash scripts/prepare-env.sh || true
	@bash scripts/build-busybox-rootfs.sh || true
	@bash scripts/build-kernel.sh || true
	@bash scripts/build-uboot.sh || true
run-qemu:
	@bash scripts/run-qemu.sh
