BOOTIMAGE_PATH:=$(abspath $(PROFILE))
OUTFILE:=$(BOOTIMAGE_PATH)/$(OUTNAME).out
BOOTIMAGE_RPRC_NAME:=$(BOOTIMAGE_PATH)/$(OUTNAME).rprc
BOOTIMAGE_NAME:=$(BOOTIMAGE_PATH)/$(OUTNAME).appimage.hs_fs
TOOL_PATH ?= ../../CPHMC_1A/tools

# Check if TOOL_PATH exists
SKIP_BOOTIMAGE := 0
ifeq ($(strip $(TOOL_PATH)),)
SKIP_BOOTIMAGE := 1
else ifeq ($(wildcard $(TOOL_PATH)),)
$(warning TOOL_PATH '$(TOOL_PATH)' does not exist, skip generating boot image)
SKIP_BOOTIMAGE := 1
endif

ifeq ($(SKIP_BOOTIMAGE),1)
all:
	@echo "Boot image generation skipped."
else
all:$(BOOTIMAGE_NAME)
endif

$(BOOTIMAGE_NAME): $(BOOTIMAGE_RPRC_NAME)
	@echo "Creating boot image: $@"
	"$(TOOL_PATH)/MulticoreImageGen.exe" LE 55 $@ $(CORE_ID) $<


$(BOOTIMAGE_RPRC_NAME): $(OUTFILE)
	@echo "Generating RPRC: $@"
	"$(TOOL_PATH)/out2rprc.exe" $< $@
