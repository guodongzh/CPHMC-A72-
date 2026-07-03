build_dir=./${PROFILE}
app_name ?= sbl_qspi_mcu1_0
out_name=${build_dir}/${app_name}


print:
	@echo "PROFILE=${PROFILE}"
	@echo "out_name=${out_name}"

all: ${out_name}.tiimage


${out_name}.tiimage: ${out_name}.bin
	powershell -executionpolicy unrestricted -command ./x509CertificateGen.ps1 -b $< -o $@ -c R5 -l 0x41C00100 -k rom_degenerateKey.pem -d DEBUG -j DBG_FULL_ENABLE -m EFUSE_DEFAULT


${out_name}.bin: ${out_name}.out
	aarch64-none-elf-objcopy --gap-fill=0xff -O binary $< $@

.PHONY: ${out_name}.out
${out_name}.out:
