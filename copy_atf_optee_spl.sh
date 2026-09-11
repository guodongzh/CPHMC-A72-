#!/usr/bin/env bash

# 复制 atf_optee_spl.appimage.hs_fs，并保留文件时间属性
set -euo pipefail

# 兼容 Git Bash（/e）和 WSL（/mnt/e）
if [[ -d /e ]]; then
    source_file='/e/work/1.Project/CPHMC_A72/A72_BareMetal/Debug/atf_optee_spl.appimage.hs_fs'
    destination_dir='/e/work/Tools/CPHMC_tool_package/a72'
elif [[ -d /mnt/e ]]; then
    source_file='/mnt/e/work/1.Project/CPHMC_A72/A72_BareMetal/Debug/atf_optee_spl.appimage.hs_fs'
    destination_dir='/mnt/e/work/Tools/CPHMC_tool_package/a72'
else
    echo '错误：找不到 E 盘挂载路径，请确认当前环境是 Git Bash 或 WSL。' >&2
    exit 1
fi

destination_file="$destination_dir/$(basename "$source_file")"

# 检查源文件
if [[ ! -f "$source_file" ]]; then
    echo "错误：源文件不存在：$source_file" >&2
    exit 1
fi

# 检查目标目录，不自动创建
if [[ ! -d "$destination_dir" ]]; then
    echo "错误：目标目录不存在：$destination_dir" >&2
    exit 1
fi

# -p 保留文件权限、所有者以及访问时间和修改时间
cp -p -- "$source_file" "$destination_file"

echo '复制完成：'
echo "  源文件：$source_file"
echo "  目标文件：$destination_file"
echo "  修改时间：$(stat -c '%y' "$destination_file" 2>/dev/null || stat -f '%Sm' "$destination_file")"

