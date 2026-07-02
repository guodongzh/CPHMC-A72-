/******************************************************************************
*@copyright  Copyright(c) 2024 RXHK.Co.,Ltd.
*@file       handle_int_prog.h
*@author     LiuRui
*@date       2025.10.21
*@brief      Provide application layer interruption templates.
*@par        History
*Date        Version   Author     Description
*2025.10.21  1.0       LiuRui     Initial version.
 ******************************************************************************/
#ifndef _HANDLE_INT_PROG_H
#define _HANDLE_INT_PROG_H

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/*                           Include Files                                   */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of constants                              */
/*---------------------------------------------------------------------------*/
#define SOFT_INT_TIME_US 1000

/*---------------------------------------------------------------------------*/
/*                      Definition of datatype                               */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*                      Definition of interface                              */
/*---------------------------------------------------------------------------*/
void handle_hard_int_prog(void);
void handle_soft_int_prog(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _HANDLE_INT_PROG_H */
