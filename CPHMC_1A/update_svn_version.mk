BUILD_PATH=$(abspath ${PROJECT_DIR}/${PROFILE})
VERSION_FILE := ${BUILD_PATH}/../app_version.h

# 默认值（无法检测到任何版本控制系统时使用）
DEFAULT_VERSION      := 1231
DEFAULT_VERSION_DATE := 2026-05-28

# 编译日期（年份只保留低两位）
BUILD_YEAR  := $(shell powershell -NoProfile -Command "[int](Get-Date -Format yy)")
BUILD_MONTH := $(shell powershell -NoProfile -Command "[int](Get-Date -Format MM)")
BUILD_DAY   := $(shell powershell -NoProfile -Command "[int](Get-Date -Format dd)")

# 编译时间
BUILD_HOUR   := $(shell powershell -NoProfile -Command "[int](Get-Date -Format HH)")
BUILD_MINUTE := $(shell powershell -NoProfile -Command "[int](Get-Date -Format mm)")
BUILD_SECOND := $(shell powershell -NoProfile -Command "[int](Get-Date -Format ss)")

# ============================================================
# 版本控制系统检测：优先 Git，其次 SVN
# ============================================================
GIT_DIR := $(shell git rev-parse --git-dir 2>/dev/null)

ifneq ($(strip $(GIT_DIR)),)
  # === Git 仓库 ===
  VCS_TYPE    := git
  # 短哈希长度 8 位：足够在任意规模的仓库中通过 git checkout <hash> 唯一回滚
  CURRENT_REV := $(shell git rev-parse --short=8 HEAD 2>/dev/null)
  LAST_REV    := $(CURRENT_REV)
  LAST_AUTHOR := $(shell git log -1 --format=%an 2>/dev/null)
  LAST_DATE   := $(shell git log -1 --format=%cs 2>/dev/null)
else
  # === 尝试 SVN 仓库 ===
  SVN_INFO := $(shell svn info 2>/dev/null)
  ifneq ($(strip $(SVN_INFO)),)
    VCS_TYPE    := svn
    CURRENT_REV := $(shell echo "$(SVN_INFO)" | awk '/Revision:/ {print $$2}')
    LAST_AUTHOR := $(shell echo "$(SVN_INFO)" | awk -F: '/Last Changed Author:/ {print $$2}' | xargs)
    LAST_REV    := $(shell echo "$(SVN_INFO)" | awk -F: '/Last Changed Rev:/ {print $$2}' | xargs)
    LAST_DATE   := $(shell echo "$(SVN_INFO)" | awk '/Last Changed Date:/ {print $$4}')
  endif
endif

# 获取失败则使用默认值
ifeq ($(strip $(CURRENT_REV)),)
  CURRENT_REV := $(DEFAULT_VERSION)
endif
ifeq ($(strip $(LAST_REV)),)
  LAST_REV := $(DEFAULT_VERSION)
endif
ifeq ($(strip $(LAST_DATE)),)
  LAST_DATE := $(DEFAULT_VERSION_DATE)
endif

# Git 场景下版本号添加 0x 前缀表示十六进制哈希值
ifeq ($(VCS_TYPE),git)
  APP_VERSION_VAL := 0x$(LAST_REV)
else
  APP_VERSION_VAL := $(LAST_REV)
endif

.PHONY: all version info clean rm_dep

all: version

rm_dep:
	@echo "Removing dependency files ..."
	@powershell -NoProfile -Command "Remove-Item -LiteralPath '${BUILD_PATH}/main.d','${BUILD_PATH}/main.o' -Force -ErrorAction SilentlyContinue; exit 0"

info: rm_dep
	@echo "VCS Type             : $(or $(VCS_TYPE),none)"
	@echo "Current Version      : $(CURRENT_REV)"
	@echo "Last Changed Author  : $(LAST_AUTHOR)"
	@echo "Last Changed Rev     : $(LAST_REV)"
	@echo "Last Changed Date    : $(LAST_DATE)"

version: rm_dep
	@echo "========================================"
	@echo "VCS Type             : $(or $(VCS_TYPE),none)"
	@echo "Current Version      : $(CURRENT_REV)"
	@echo "Last Changed Rev     : $(LAST_REV)"
	@echo "Last Changed Date    : $(LAST_DATE)"
	@echo "Build Date           : $(BUILD_YEAR)-$(BUILD_MONTH)-$(BUILD_DAY)"
	@echo "Build Time           : $(BUILD_HOUR):$(BUILD_MINUTE):$(BUILD_SECOND)"
	@echo "========================================"

	$(file >$(VERSION_FILE),#ifndef _APP_VERSION_H)
	$(file >>$(VERSION_FILE),#define _APP_VERSION_H)
	$(file >>$(VERSION_FILE),)
	$(file >>$(VERSION_FILE),#define APP_VSC_VERSION ($(APP_VERSION_VAL)))
	$(file >>$(VERSION_FILE),#define APP_VSC_DATE    "$(LAST_DATE)")
	$(file >>$(VERSION_FILE),)
	$(file >>$(VERSION_FILE),#define APP_BUILD_YEAR   ($(BUILD_YEAR)))
	$(file >>$(VERSION_FILE),#define APP_BUILD_MONTH  ($(BUILD_MONTH)))
	$(file >>$(VERSION_FILE),#define APP_BUILD_DAY    ($(BUILD_DAY)))
	$(file >>$(VERSION_FILE),)
	$(file >>$(VERSION_FILE),#define APP_BUILD_HOUR   ($(BUILD_HOUR)))
	$(file >>$(VERSION_FILE),#define APP_BUILD_MINUTE ($(BUILD_MINUTE)))
	$(file >>$(VERSION_FILE),#define APP_BUILD_SECOND ($(BUILD_SECOND)))
	$(file >>$(VERSION_FILE),)
	$(file >>$(VERSION_FILE),#endif)
	@echo "$(VERSION_FILE) updated."

clean:
	@powershell -NoProfile -Command "Remove-Item -LiteralPath '$(VERSION_FILE)' -Force -ErrorAction SilentlyContinue; exit 0"
