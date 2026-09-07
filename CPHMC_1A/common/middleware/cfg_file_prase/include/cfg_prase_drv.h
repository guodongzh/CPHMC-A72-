/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       cfg_prase_drv.h
 *@author     wenjunf
 *@date       2026.01.22
 *@brief      配置文件解析驱动接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.22  1.0       wenjunf    example
 ******************************************************************************/
#ifndef _CFG_PRASE_DRV_H
#define _CFG_PRASE_DRV_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/
typedef struct _tIniS2
{
    char           *s2;
    int             ChildCount;
    char           *value;
    struct _tIniS2 *next;
} tIniS2;

typedef struct _tIniS1
{
    char           *s1;
    int             ChildCount;
    tIniS2         *pChildren;
    struct _tIniS1 *next;
} tIniS1;

typedef struct _tIniHandle
{
    const char *iniString;
    const char *iniLine;
    const char *iniEnd;  // 配置字符串结束边界(指向最后一个有效字符之后)，防止越界读
    tIniS1     *s1List;
    int         ChildCount;
    char       *buff;
    int         buffFree;
} tIniHandle;
/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void *ini_GetAddr32(char *p, int *pSize8);  // 从buff取得32位对齐地址
int   ini_readLine(char **ppLine);          // 读行，返回1=表头[s1]，2=数据，0=结束，-1=空间溢出
int   ini_str2s2(char *str, tIniS2 *ps2);   // 将字符串分解为tIniS2，返回以","分隔的参数数目

/* ini initialize */
/* 0=success,-1=buff overflow,-2=fault,-3=参数非法 */
/* buffLength为buff的字节数(byte)，建议不小于(32+strlen(pIniString)) */
/* 完成此函数后，调用方可释放pIniString空间 */
int ini_Initialize(const char *pIniString, int *buff, int buffLength);

/* 读取字符串参数，无对应键值时，返回0 */
/* *pChildCount为值域由“,”分裂的字符串数目*/
int8_t *ini_GetVarStr(const char *s1, const char *s2, int32_t *pChildCount);

/* 读取整型参数，无对应键值时，返回0 */
int ini_GetVarInt(const char *s1, const char *s2);

/* 读取浮点参数，无对应键值时，返回0.0 */
float ini_GetVarFloat(const char *s1, const char *s2);

/* 释放内存(可不调用) */
/* 完成此函数后，调用方可释放buff */
/* 调用此函数之前，ini配置读取函数允许全局使用 */
void ini_Finalize(void);

/* 从str读取整数(支持0x方式的16进制) */
int ini_str2hex(const char *str);

/* 从str读取浮点数 */
float ini_str2float(const char *str);

/* 读取被字符split分列的第n个字段 */
int8_t *ini_splitStr(const char *str, char split, int16_t n, int32_t *pLength);
int8_t *ini_splitStr_chinese(const char *str, char split, int16_t n, int32_t *pLength);
#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _CFG_PRASE_DRV_H */