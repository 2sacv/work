
PRO_DRIVER_DIR  = $(shell pwd)
INSTALL_DIR	= $(PRO_DRIVER_DIR)/../filesys/filesys_normal/ipc/drv
KERNEL_DIR 	= $(PRO_DRIVER_DIR)/../kernel/linux-5.10.221
DRIVER_DIR  = $(PRO_DRIVER_DIR)

CROSS_COMPILE = arm-v01c02-linux-musleabi-

export CROSS_COMPILE
export INSTALL_DIR
export KERNEL_DIR
export DRIVER_DIR
export SDKLIBS

INSTALLCHECK:
ifdef INSTALL_DIR
	@echo "=== install dir is $(DRIVER_DIR) ==="
else
	$(warning "=== INSTALL_DIR not set, please set env INSTALL_DIR first ===")
	$(warning "=== you can "export INSTALL_DIR=your dest dir" ===")
	$(warning "=== or set "INSTALL_DIR=your dest dir" at .bash_profile, and so on. ===")
	$(error "=== INSTALL_DIR ===")
endif

KERNELCHECK:
ifdef KERNEL_DIR
	@echo "=== kernel dir is $(KERNEL_DIR) ==="
else
	$(warning "=== KERNEL_DIR not set, please set env KERNEL_DIR first ===")
	$(warning "=== you can "export KERNEL_DIR_DM365=your dest dir" ===")
	$(warning "=== or set "KERNEL_DIR=your dest dir" at .bash_profile, and so on. ===")
	$(error "=== KERNEL_DIR ===")
endif

.PHONY : $(INSTALLCHECK) $(KERNELCHECK)
