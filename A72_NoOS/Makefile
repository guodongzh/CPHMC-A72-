# CCS manual-makefile entry points.
# The actual build logic stays in build.ps1 so command-line and CCS builds
# produce exactly the same ELF/RPRC/appimage files.

.PHONY: all wkup main clean clean-wkup clean-main rebuild help

POWERSHELL ?= powershell.exe
BUILD_SCRIPT := $(CURDIR)/build.ps1

all: wkup

wkup:
	$(POWERSHELL) -NoLogo -NoProfile -ExecutionPolicy Bypass -File "$(BUILD_SCRIPT)" -Console Wkup

main:
	$(POWERSHELL) -NoLogo -NoProfile -ExecutionPolicy Bypass -File "$(BUILD_SCRIPT)" -Console Main

clean: clean-wkup clean-main

clean-wkup:
	$(POWERSHELL) -NoLogo -NoProfile -ExecutionPolicy Bypass -File "$(BUILD_SCRIPT)" -Console Wkup -Clean

clean-main:
	$(POWERSHELL) -NoLogo -NoProfile -ExecutionPolicy Bypass -File "$(BUILD_SCRIPT)" -Console Main -Clean

rebuild: clean all

help:
	@echo all/wkup  - build A72_0 image using WKUP_UART0 (default)
	@echo main      - build A72_0 image using MAIN_UART0
	@echo clean     - remove both output variants
	@echo rebuild   - clean, then build the default WKUP_UART0 variant
