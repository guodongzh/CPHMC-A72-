/**
 *************************************************************************
 * @file      ipc_scada_data.h
 * @author    zht
 * @date      2023/11/08
 * @brief     ipc scada interface 
 * @attention None
 *************************************************************************
 */
#ifndef IPC_SCADA_DATA_H_
#define IPC_SCADA_DATA_H_

#ifdef __cplusplus
extern "C"
{
#endif
#ifdef BUILD_MCU

#include <stdint.h>
#include <string.h>

// 四遥, 共 4096 bytes
#define IPC_YX_MAX_BUF_SIZE_WORDS  (64)   /* Max size of YX, in words, 128 bytes */
#define IPC_YK_MAX_BUF_SIZE_WORDS  (64)   /* Max size of YK, in words, 128 bytes */
#define IPC_YC_MAX_BUF_SIZE_FLOAT  (512)  /* Max size of YC, in float, 2048 bytes */
#define IPC_YT_MAX_BUF_SIZE_FLOAT  (256)  /* Max size of YT, in float, 1024 bytes */
#define IPC_RES1_SIZE_BYTES        (640)  /* 备用640 bytes, 以上共4096 */

typedef struct
{
    uint16_t yx_num;
    uint16_t yk_num;
    uint16_t yc_num;
    uint16_t yt_num;

} IPC_SCADA_CFG_t;

extern IPC_SCADA_CFG_t gIpcScadaCfg;
extern uint16_t gIpcYkData[IPC_YK_MAX_BUF_SIZE_WORDS + 1];
extern float gIpcYtData[IPC_YT_MAX_BUF_SIZE_FLOAT + 1];
extern uint8_t gIpcYtData_flag[(IPC_YT_MAX_BUF_SIZE_FLOAT / 8) + 1];
extern uint16_t gIpcYxData[IPC_YX_MAX_BUF_SIZE_WORDS];
extern uint16_t gIpcYxDataBak[IPC_YX_MAX_BUF_SIZE_WORDS];

// 四遥
// 配置信息
void scada_cfg_init(uint16_t yxNum, uint16_t ykNum, uint16_t ycNum, uint16_t ytNum);
void get_scada_cfg(uint16_t *yxNum, uint16_t *ykNum, uint16_t *ycNum, uint16_t *ytNum);

// 写遥信
void write_yx_1b(uint16_t grp_ofs, uint8_t bitNo, uint8_t v1);
void write_yx_1w(uint16_t grp_ofs, uint16_t v1);
void write_yx_16b(uint16_t grp_ofs, uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,  uint8_t v8,
                              uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15, uint8_t v16);

// 写遥测
void write_yc_1r(uint16_t ofs, float v1);
void write_yc_3r(uint16_t ofs, float v1, float v2, float v3);
void write_yc_6r(uint16_t ofs, float v1, float v2, float v3, float v4, float v5, float v6);
void write_yc_8r(uint16_t ofs, float v1, float v2, float v3, float v4, float v5, float v6, float v7, float v8);

// 读遥调
void read_yt_1r(uint16_t ofs, float *v1, uint8_t *isvalid);
void read_yt_8r(uint16_t ofs, float *v1, float *v2, float *v3, float *v4, float *v5, float *v6, float *v7, float *v8,
                uint8_t *isvalid1, uint8_t *isvalid2, uint8_t *isvalid3, uint8_t *isvalid4,
                uint8_t *isvalid5, uint8_t *isvalid6, uint8_t *isvalid7, uint8_t *isvalid8);

//写遥控
void read_yk(uint16_t ofs, uint8_t *open, uint8_t *close);

#ifdef __cplusplus
}
#endif

#endif

#endif