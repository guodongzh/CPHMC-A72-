/******************************************************************************
 *@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
 *@file       cfg_prase_drv.c
 *@author     wenjunf
 *@date       2026.01.22
 *@brief      配置文件解析驱动接口
 *@par        History
 *Date        Version   Author     Description
 *2026.01.22  1.0       wenjunf    example
 ******************************************************************************/

/*---------------------------------------------------------------------------*/
/*                             Include Files                                 */
/*---------------------------------------------------------------------------*/
#include "cfg_prase_drv.h"
#if !defined(CORE_R5F1) || !defined(SMARTDEV_FUNC)
/* -------------------------------------------------------------------------- */
/*                            Global Variables                                */
/* -------------------------------------------------------------------------- */
tIniHandle *g_hIni = 0;
/* -------------------------------------------------------------------------- */
/*                          Function Declarations                             */
/* -------------------------------------------------------------------------- */

/* -------------------------------------------------------------------------- */
/*                          Function Definitions                              */
/* -------------------------------------------------------------------------- */
/* ini initialize */
/* 0=success,-1=buff overflow,-2=fault */
/* 建议buff的空间为(32+strlen(pIniString)), sizeof(int) */
/* 完成此函数后，调用方可释放pIniString空间 */
int ini_Initialize(const char *pIniString, int *buff, int buffLength)
{
    tIniS1 *ps1;
    tIniS2 *ps2 = NULL;
    tIniS2 *p;
    char   *str;
    int     i, size, len;

    /* Bug5: 参数合法性检查，避免空指针/非法长度导致越界 */
    if ((0 == pIniString) || (0 == buff) || (buffLength <= 0))
    {
        g_hIni = 0;
        return -3;
    }

    str = (char *)buff;
    /* Bug1: buffLength由调用方以字节数(byte)传入，不能再乘sizeof(int32_t)，
     * 否则len被高估4倍，解析数据会越界写出buff缓冲区 */
    len = buffLength;
    /* buff至少需容纳tIniHandle + tIniS1 + 对齐余量，否则视为溢出 */
    if (len < (int)(sizeof(tIniHandle) + sizeof(tIniS1) + 2 * sizeof(int) + 1))
    {
        g_hIni = 0;
        return -1;
    }
    // 初始化配置环境
    size   = sizeof(tIniHandle);
    g_hIni = (tIniHandle *)ini_GetAddr32(str, &size);
    str += size;
    len -= size;
    size = sizeof(tIniS1);
    ps1  = (tIniS1 *)ini_GetAddr32(str, &size);
    str += size;
    len -= size;
    ps1->s1            = 0;
    ps1->ChildCount    = 0;
    ps1->pChildren     = 0;
    ps1->next          = 0;
    g_hIni->iniString  = pIniString;
    g_hIni->iniLine    = pIniString;
    /* Bug3: 记录配置字符串结束边界，ini_readLine据此防止越界读 */
    g_hIni->iniEnd     = pIniString + strlen(pIniString);
    g_hIni->s1List     = ps1;
    g_hIni->ChildCount = 0;
    g_hIni->buff       = str;
    g_hIni->buffFree   = --len;

    // 缓存配置信息
    i = ini_readLine(&str);
    while (i)
    {
        switch (i)
        {
        case 1:  // title(s1)
            if (0 != g_hIni->ChildCount++)
            {
                size      = sizeof(tIniS1);
                ps1->next = (tIniS1 *)ini_GetAddr32(g_hIni->buff, &size);
                if (size > g_hIni->buffFree)
                {
                    return -1;
                }
                g_hIni->buffFree -= size;
                g_hIni->buff += size;
                ps1             = ps1->next;
                ps1->ChildCount = 0;
                ps1->pChildren  = 0;
                ps1->next       = 0;
            }
            ps1->s1 = str;
            break;

        case 2:  // value(s2=)
            if (0 == g_hIni->ChildCount)
                break;
            size = sizeof(tIniS2);
            p    = (tIniS2 *)ini_GetAddr32(g_hIni->buff, &size);
            //  ps2 = p;
            if (size > g_hIni->buffFree)
            {
                return -1;
            }
            g_hIni->buffFree -= size;
            g_hIni->buff += size;
            p->ChildCount = ini_str2s2(str, p);
            if (p->ChildCount)
            {
                /* Bug4: 仅当本section已有子节点且ps2非空时才链接next，
                 * 防止异常流程下解引用未初始化的ps2 */
                if ((0 != ps1->ChildCount++) && (0 != ps2))
                {
                    ps2->next = p;
                    ps2       = p;
                }
                else
                {
                    ps1->pChildren = p;
                    ps2            = p;
                }
            }
            break;

        default:
            return (i);
        }
        i = ini_readLine(&str);
    }

    // 结束
    if (0 == g_hIni->ChildCount)
    {
        g_hIni = 0;
        return -2;
    }

    return 0;
}

/* 读取字符串参数 */
int8_t *ini_GetVarStr(const char *s1, const char *s2, int32_t *pChildCount)
{
    tIniS1 *ps1;
    tIniS2 *ps2;
    int     i, j, num;

    if (0 != pChildCount)
        *pChildCount = 0;
    /* Bug5: 入参及全局句柄空指针检查 */
    if ((0 == g_hIni) || (0 == s1) || (0 == s2))
        return 0;

    ps1 = g_hIni->s1List;
    num = g_hIni->ChildCount;
    for (i = 0; i < num; i++)
    {
        /* Bug2: 链表节点或表头键为空时跳过，避免strcmp(NULL,...)崩溃 */
        if (0 == ps1)
            break;
        if ((0 != ps1->s1) && (0 == strcmp((const char *)s1, (char *)ps1->s1)))
        {
            ps2 = ps1->pChildren;
            num = ps1->ChildCount;
            for (j = 0; j < num; j++)
            {
                /* Bug2: 子节点或键名为空时跳过，避免strcmp(NULL,...)崩溃 */
                if (0 == ps2)
                    break;
                if ((0 != ps2->s2) && (0 == strcmp((const char *)s2, (char *)ps2->s2)))
                {
                    if (0 != pChildCount)
                        *pChildCount = ps2->ChildCount;
                    return ((int8_t *)ps2->value);
                }
                ps2 = ps2->next;
            }
            break;
        }
        ps1 = ps1->next;
    }
    return 0;
}

/* 从str读取整数(支持0x方式的16进制) */
int ini_str2hex(const char *str)
{
    int result = 0;

    if (0 != str)
    {
        if ('\0' != str[0])
        {
            if (('0' == str[0]) && ('x' == str[1]))
            {
                sscanf(str, "%x", &result);
            }
            else
                sscanf(str, "%d", &result);
        }
    }
    return (result);
}

/* 读取整型参数 */
int ini_GetVarInt(const char *s1, const char *s2)
{
    return (ini_str2hex((char *)ini_GetVarStr(s1, s2, 0)));
}

/* 从str读取浮点数 */
float ini_str2float(const char *str)
{
    float result = (float)0.0f;

    if (0 != str)
    {
        if ('\0' != str[0])
        {
            sscanf((const char *)str, "%f", &result);
        }
    }
    return (result);
}

/* 读取浮点参数 */
float ini_GetVarFloat(const char *s1, const char *s2)
{
    return (ini_str2float((char *)ini_GetVarStr(s1, s2, 0)));
}

/* 释放内存 */
/* 完成此函数后，调用方可释放buff */
void ini_Finalize(void)
{
    g_hIni = 0;
}

/* 从buff取得32位对齐地址 */
void *ini_GetAddr32(char *p, int *pSize8)
{
    int i, j;

    i = (int)p % sizeof(int);
    if (0 != i)
    {
        j = sizeof(int) - i;
        *pSize8 += j;
        return (p + j);
    }
    return (p);
}

/* 读取一行有效字符串 */
/* 丢弃空行及";"以后部份 */
/* 返回1=表头[s1]，2=数据，0=结束，-1=空间溢出 */
int ini_readLine(char **ppLine)
{
    int         num, i = 0;
    const char *str;
    const char *strEnd;
    char       *p;
    char        ch;

    if (0 == g_hIni)
        return 0;
    str = (const char *)g_hIni->iniLine;
    if (0 == str)
        return 0;
    /* Bug3: 配置字符串结束边界，防止源串无'\0'终止时越界读 */
    strEnd  = g_hIni->iniEnd;
    num     = g_hIni->buffFree;
    p       = g_hIni->buff;
    *ppLine = p;

    ch = *str;
    while (ch)
    {
        /* Bug3: 到达结束边界即停止读取 */
        if (str >= strEnd)
            break;
        ch = *str++;
        if ('\0' == ch)
            break;
        if ((uint8_t)ch == 0xff)
            break;
        if (';' == ch)  // 忽略注释内容
        {
            ch = *str;
            while (ch)
            {
                /* Bug3: 注释跳过同样受结束边界约束 */
                if (str >= strEnd)
                    break;
                ch = *str++;
                if (('\r' == ch) || ('\n' == ch))
                    break;
            }
            if ('\0' == ch)
                break;
        }
        if (('\r' == ch) || ('\n' == ch))
        {
            while (0 != i)
            {
                ch = p[--i];
                if (' ' < (unsigned char)ch)  // 往前清除空白
                {
                    if (('[' == p[0]) && (']' == ch))
                    {
                        if (2 > i)
                        {
                            i = 0;
                            break;
                        }
                        (*ppLine)++;
                        p[i] = '\0';
                        g_hIni->buffFree -= ++i;
                        g_hIni->buff    = &p[i];
                        g_hIni->iniLine = str;
                        return 1;
                    }
                    p[++i] = '\0';
                    g_hIni->buffFree -= ++i;
                    g_hIni->buff    = &p[i];
                    g_hIni->iniLine = str;
                    return 2;
                }
            }
            continue;
        }

        // 忽略行前空白
        if ((0 == i) && (' ' >= (unsigned char)ch))
            continue;
        p[i++] = ch;  // 拷贝字符
        if (i >= num)
        {
            return -1;
        }
    }

    while (0 != i)
    {
        ch = p[--i];
        if (' ' < (unsigned char)ch)  // 往前清除空白
        {
            if (('[' == p[0]) && (']' == ch))
            {
                if (2 > i)
                    break;
                (*ppLine)++;
                p[i] = '\0';
                g_hIni->buffFree -= ++i;
                g_hIni->buff    = &p[i];
                g_hIni->iniLine = str;
                return 1;
            }
            p[++i] = '\0';
            g_hIni->buffFree -= ++i;
            g_hIni->buff    = &p[i];
            g_hIni->iniLine = str;
            return 2;
        }
    }
    g_hIni->iniLine = str;
    return 0;
}

/* 将字符串分解为tIniS2 */
/* 返回以","分隔的参数数目 */
int ini_str2s2(char *str, tIniS2 *ps2)
{
    unsigned char *p;
    unsigned char  ch;
    int            i = 0;
    int            j = 1;
    int            k;

    /* Bug5: 入参空指针检查 */
    if ((0 == str) || (0 == ps2))
        return 0;

    p               = (unsigned char *)str;
    ps2->ChildCount = 0;
    ps2->next       = 0;
    // 读取s2
    ps2->s2 = str;
    ch      = p[i];
    while (ch)
    {
        ch = p[i++];
        if ('=' == ch)
        {
            for (k = --i; k > 0;)
            {
                if (' ' < p[--k])
                {
                    str[++k] = '\0';
                    break;
                }
            }
            if (0 == k)
                return 0;
            ch = p[++i];
            while (ch)
            {
                if (' ' < ch)
                    break;
                ch = p[++i];
            }
            break;
        }
    }
    if ('\0' == ch)
        return 0;
    // 读取value
    ps2->value = str + i;
    ch         = p[i];
    while (ch)
    {
        ch = p[i++];
        if (',' == ch)
            j++;
    }
    ps2->ChildCount = j;
    return (j);
}

/* 读取被字符(ASCII)split分列的第n个字段 */
int8_t *ini_splitStr(const char *str, char split, int16_t n, int32_t *pLength)
{
    int   i = 0;
    char *p;
    char  ch;

    /* Bug5: 入参空指针检查 */
    if ((0 == str) || (0 == pLength))
        return 0;

    p = (char *)str;
    if (0 < n)  // 找到第n个字段
    {
        ch = *p;
        while (ch)
        {
            ch = *p++;
            if (ch == split)
            {
                if (n == ++i)
                    break;
            }
        }
        if (i != n)
        {
            *pLength = 0;
            return 0;
        }
        i = 0;
    }
    ch = p[i];
    while (ch)
    {
        ch = p[i++];
        if (ch == split)
            break;
        if ('\0' == ch)
        {
            if (0 < i)
                break;
            *pLength = 0;
            return 0;
        }
    }

#if 1
    while (' ' >= *p)  // 去除字符串前空白
    {
        if (1 >= i)
            break;
        p++;
        i--;
    }
#endif
    /* Bug6: i为0时--i会得到-1，此处做下限保护 */
    *pLength = (i > 0) ? (i - 1) : 0;
    return ((int8_t *)p);
}

/* 读取被字符（汉字）split分列的第n个字段 */
int8_t *ini_splitStr_chinese(const char *str, char split, int16_t n, int32_t *pLength)
{
    int   i = 0;
    char *p;
    char  ch;

    /* Bug5: 入参空指针检查 */
    if ((0 == str) || (0 == pLength))
        return 0;

    p = (char *)str;
    if (0 < n)  // 找到第n个字段
    {
        ch = *p;
        while (ch)
        {
            ch = *p++;
            if (ch == split)
            {
                if (n == ++i)
                    break;
            }
        }
        if (i != n)
        {
            *pLength = 0;
            return 0;
        }
        i = 0;
    }
    ch = p[i];
    while (ch)
    {
        ch = p[i++];
        if (ch == split)
            break;
        if ('\0' == ch)
        {
            if (0 < i)
                break;
            *pLength = 0;
            return 0;
        }
    }

    while (' ' >= (uint8_t)(*p))  // 去除字符串前空白
    {
        if (1 >= i)
            break;
        p++;
        i--;
    }

    /* Bug6: i为0时--i会得到-1，此处做下限保护 */
    *pLength = (i > 0) ? (i - 1) : 0;
    return ((int8_t *)p);
}

#endif