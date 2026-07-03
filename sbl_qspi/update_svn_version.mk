BUILD_PATH=$(abspath ${PROFILE})
VERSION_FILE := app_version.h

# 默认值
DEFAULT_SVN_REV  := 1231
DEFAULT_SVN_DATE := 2026-05-28

# 编译日期（年份只保留低两位）
BUILD_YEAR  := $(shell date +%y)
BUILD_MONTH := $(shell date +%m)
BUILD_DAY   := $(shell date +%d)

# 编译时间
BUILD_HOUR   := $(shell date +%H)
BUILD_MINUTE := $(shell date +%M)
BUILD_SECOND := $(shell date +%S)

# 获取 SVN 信息
CURRENT_REV := $(shell svn info 2>/dev/null | awk '/Revision:/ {print $$2}')
LAST_AUTHOR := $(shell svn info 2>/dev/null | awk -F: '/Last Changed Author:/ {print $$2}' | xargs)
LAST_REV    := $(shell svn info 2>/dev/null | awk -F: '/Last Changed Rev:/ {print $$2}' | xargs)
LAST_DATE   := $(shell svn info 2>/dev/null | awk '/Last Changed Date:/ {print $$4}')

# 获取失败则使用默认值
ifeq ($(strip $(CURRENT_REV)),)
CURRENT_REV := $(DEFAULT_SVN_REV)
endif

ifeq ($(strip $(LAST_REV)),)
LAST_REV := $(DEFAULT_SVN_REV)
endif

ifeq ($(strip $(LAST_DATE)),)
LAST_DATE := $(DEFAULT_SVN_DATE)
endif

.PHONY: all version info clean rm_dep

all: version

rm_dep:
	@echo "Removing dependency files..."
	rm -f ${BUILD_PATH}/main.d ${BUILD_PATH}/main.o

info: rm_dep
	@echo "Current SVN revision : $(CURRENT_REV)"
	@echo "Last Changed Author  : $(LAST_AUTHOR)"
	@echo "Last Changed Rev     : $(LAST_REV)"
	@echo "Last Changed Date    : $(LAST_DATE)"

version: rm_dep
	@echo "========================================"
	@echo "Current SVN revision : $(CURRENT_REV)"
	@echo "Last Changed Rev     : $(LAST_REV)"
	@echo "Last Changed Date    : $(LAST_DATE)"
	@echo "Build Date           : $(BUILD_YEAR)-$(BUILD_MONTH)-$(BUILD_DAY)"
	@echo "Build Time           : $(BUILD_HOUR):$(BUILD_MINUTE):$(BUILD_SECOND)"
	@echo "========================================"

	@if [ ! -f $(VERSION_FILE) ]; then \
		echo "#ifndef _APP_VERSION_H" > $(VERSION_FILE); \
		echo "#define _APP_VERSION_H" >> $(VERSION_FILE); \
		echo "" >> $(VERSION_FILE); \
		echo "#define APP_SVN_VERSION ($(DEFAULT_SVN_REV))" >> $(VERSION_FILE); \
		echo "#define APP_SVN_DATE    \"$(DEFAULT_SVN_DATE)\"" >> $(VERSION_FILE); \
		echo "" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_YEAR   ($(BUILD_YEAR))" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_MONTH  ($(BUILD_MONTH))" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_DAY    ($(BUILD_DAY))" >> $(VERSION_FILE); \
		echo "" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_HOUR   ($(BUILD_HOUR))" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_MINUTE ($(BUILD_MINUTE))" >> $(VERSION_FILE); \
		echo "#define APP_BUILD_SECOND ($(BUILD_SECOND))" >> $(VERSION_FILE); \
		echo "" >> $(VERSION_FILE); \
		echo "#endif" >> $(VERSION_FILE); \
		echo "$(VERSION_FILE) created."; \
	else \
		echo "$(VERSION_FILE) already exists."; \
	fi

	@if [ "$(LAST_REV)" != "$(DEFAULT_SVN_REV)" ]; then \
		sed -i 's/^#define APP_SVN_VERSION.*/#define APP_SVN_VERSION ($(LAST_REV))/g' $(VERSION_FILE); \
		echo "APP_SVN_VERSION updated."; \
	fi

	@if [ "$(LAST_DATE)" != "$(DEFAULT_SVN_DATE)" ]; then \
		sed -i 's/^#define APP_SVN_DATE.*/#define APP_SVN_DATE    \"$(LAST_DATE)\"/g' $(VERSION_FILE); \
		echo "APP_SVN_DATE updated."; \
	fi

	@sed -i 's/^#define APP_BUILD_YEAR.*/#define APP_BUILD_YEAR   ($(BUILD_YEAR))/g' $(VERSION_FILE)
	@sed -i 's/^#define APP_BUILD_MONTH.*/#define APP_BUILD_MONTH  ($(BUILD_MONTH))/g' $(VERSION_FILE)
	@sed -i 's/^#define APP_BUILD_DAY.*/#define APP_BUILD_DAY    ($(BUILD_DAY))/g' $(VERSION_FILE)

	@sed -i 's/^#define APP_BUILD_HOUR.*/#define APP_BUILD_HOUR   ($(BUILD_HOUR))/g' $(VERSION_FILE)
	@sed -i 's/^#define APP_BUILD_MINUTE.*/#define APP_BUILD_MINUTE ($(BUILD_MINUTE))/g' $(VERSION_FILE)
	@sed -i 's/^#define APP_BUILD_SECOND.*/#define APP_BUILD_SECOND ($(BUILD_SECOND))/g' $(VERSION_FILE)

	@echo "Build date/time updated."

clean:
	rm -f $(VERSION_FILE)