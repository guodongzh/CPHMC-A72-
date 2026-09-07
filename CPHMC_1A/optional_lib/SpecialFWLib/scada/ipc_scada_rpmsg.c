/**
 *************************************************************************
 * @file      ipc_scada_rpmsg.c
 * @author    zht
 * @date      2023/11/08
 * @brief     ipc for scada rpmessage, for yt, yk
 * @attention None
 *************************************************************************
 */

/* ========================================================================== */
/*                             Include Files                                  */
/* ================================ww========================================== */
#ifdef BUILD_MCU

#include "ipc_scada_rpmsg.h"
#include "ipc_scada_data.h"
#include "debug_config.h"
#include "ti/drv/ipc/ipc.h"
#include "bsp/ipc/ipc_cfg.h"
#include "ti/osal/TimerP.h"
#include "irig_b_interface.h"

/* ========================================================================== */
/*                            Global Variables                                */
/* ========================================================================== */
static RPMessage_Handle scada_handle;
static uint8_t scada_rpmsg_object_buf[RPMSG_DATA_SIZE] __attribute__((section(".ipc_data_buffer"), aligned(8)));

/* ========================================================================== */
/*                          Function Definitions                              */
/* ========================================================================== */
#define YK_TIMER_NUM 1
static yk_Timer g_ykTimer[YK_TIMER_NUM];

static void yk_timer_call_back(uintptr_t arg)
{
    uint16_t ofs;
    uint8_t bitNo;

    yk_Timer *pYk_Timer = (yk_Timer *)arg;

    ofs = pYk_Timer->ofs;
    bitNo = pYk_Timer->bit_no;
    pYk_Timer->is_active = false;
    gIpcYkData[ofs] &= ~(1 << bitNo);
}

static void init_yk_timer(void)
{
    TimerP_Params timerParams;

    /* create harderwear timer */
    TimerP_Params_init(&timerParams);
    timerParams.runMode = TimerP_RunMode_ONESHOT;
    timerParams.startMode = TimerP_StartMode_USER;
    timerParams.periodType = TimerP_PeriodType_MICROSECS;
    timerParams.period = 5000;  // 5s

    for (int i = 0; i < YK_TIMER_NUM; i++)
    {
        timerParams.arg =  &g_ykTimer[i];
        g_ykTimer->is_active = false;
        g_ykTimer[i].timer_handle = TimerP_create(TimerP_ANY,
                                                  yk_timer_call_back,
                                                  &timerParams);
    }

}

static bool is_yk_opering(uint16_t ptNr)
{
    uint8_t i;

    for (i = 0; i < YK_TIMER_NUM; i++)
    {
        if (g_ykTimer[i].pt_nr == ptNr &&
            (g_ykTimer[i].is_active == true))
        {
            return (true);
        }
    }

    return false;
}

static int8_t get_yk_free_timer(void)
{
    int8_t i;

    for (i = 0; i < YK_TIMER_NUM; i++)
    {
        if (g_ykTimer[i].is_active == false)
        {
            return i;
        }
    }

    return (-1);
}

static void start_ykTimer(yk_Timer *pTimer)
{
    TimerP_setPeriodMicroSecs(pTimer->timer_handle, 5000000);
    pTimer->is_active = true;
    TimerP_start(pTimer->timer_handle);
}

static int8_t check_Yk1_Order(MSG_YK_1_t *pYk, int8_t *timer)
{
    int8_t errCode = 0;
    int8_t i;

    // check.
    if ((pYk->val_pos == (uint8_t)(~(pYk->val_inv))) &&
        ((pYk->val_pos == YK_VAL_1_POS) || (pYk->val_pos == YK_VAL_0_POS)))
    {
        if (pYk->Nr >= gIpcScadaCfg.yk_num)
        {
            errCode |= ERRCODE_YK_PTNR_INVALID;
            return errCode;
        }

        if (is_yk_opering(pYk->Nr))  // 5s内不接受同一个点的重复遥控指令
        {
            errCode |= ERRCODE_YK_IS_OPERING;
            return errCode;
        }

        i = get_yk_free_timer();
        if (i < 0)
        {
            errCode |= ERRCODE_YK_TIMER_BUSY;
            return errCode;
        }

        *timer = i;

        return errCode;
    }
    else
    {
        errCode |= ERRCODE_YK_ORD_INVALID;
        return errCode;
    }
}

void scada_init()
{
    RPMessage_Params params;
    uint32_t myEndPt = 0;
    int32_t status = 0;

    RPMessageParams_init(&params);
    params.requestedEndpt = LOCAL_ENDPT_FOR_SCADA_RECV;
    params.buf = scada_rpmsg_object_buf;
    params.bufSize = sizeof(scada_rpmsg_object_buf);

    scada_handle = RPMessage_create(&params, &myEndPt);
    if (!scada_handle)
    {
        IPC_log("RecvTask: Failed to create endpoint\n");
        return;
    }

    status = RPMessage_announce(RPMESSAGE_ALL, myEndPt, "rpmsg_chrdev");
    if (status != IPC_SOK)
    {
        IPC_log("RecvTask: RPMessage_announce() for %s failed\n", "rpmsg_chrdev");
        return;
    }

    init_yk_timer();
}

void yt_yk_task(void)
{
    IPC_SCADA_MSG_t recv_data;
    IPC_SCADA_MSG_t send_data;
    int32_t status;
    uint16_t len;
    uint32_t remote_endpt;
    uint32_t remote_core_id;
    MSG_YT_1_t *yt1;
    MSG_YT_N_t *ytN;
    MSG_YK_1_t *yk1;
    MSG_YK_N_t *ykN;
    MSG_ACK_t *pAck = (MSG_ACK_t *)&send_data.data[0];
    uint32_t sum_check;
    uint8_t flag = 1;
    uint8_t errCode = 0;
    int8_t timerNo;
    uint16_t ofs;
    uint8_t bitNo;

    memset(&send_data, 0, sizeof(IPC_SCADA_MSG_t));
    send_data.dst = IPC_MPU1_0;
    send_data.src = Ipc_mpGetSelfId();;
    send_data.typ = IPC_SCADA_MSG_ACK;
    send_data.len = sizeof(MSG_ACK_t);

    while (1)
    {
        len = sizeof(IPC_SCADA_MSG_t);
        memset(&recv_data, 0, sizeof(IPC_SCADA_MSG_t));

        status = RPMessage_recv(scada_handle,
                                &recv_data,
                                &len,
                                &remote_endpt,
                                &remote_core_id,
                                0);

        if (status != IPC_SOK)
        {
//            Debug_logError("failed with code %d\n", status);
            break;
        }
        else
        {
            SCADA_log("%s <--> %s: scada ipc recvd \n",
                      Ipc_mpGetSelfName(),
                      Ipc_mpGetName(remote_core_id));
            sum_check = 0;
            for (int i = 0; i < recv_data.len; ++i)
            {
                sum_check += recv_data.data[i];
            }
            if (sum_check == recv_data.sum)
            {
                switch (recv_data.typ)
                {
                case IPC_SCADA_MSG_CMD_YT_1:  // 单点遥调指令
                    yt1 = (MSG_YT_1_t *)recv_data.data;
                    SCADA_log("single yt, yt1->Nr=%d %f\r\n", yt1->Nr, yt1->val);
                    if (yt1->Nr < gIpcScadaCfg.yt_num)
                    {
                        gIpcYtData[yt1->Nr] = yt1->val;
                        gIpcYtData_flag[yt1->Nr / 8] |= 0x01 << (yt1->Nr % 8);
                    }
                    pAck->cmd = IPC_SCADA_MSG_CMD_YT_1;
                    pAck->ack = ACK_SUCCED;
                    pAck->num = 1;
                    flag = 1;
                    break;
                case IPC_SCADA_MSG_CMD_YT_N:  // 批量遥调
                    ytN = (MSG_YT_N_t *)recv_data.data;
                    SCADA_log("bulk yt, ");
                    for (int i = 0; i < ytN->num; ++i)
                    {
                        if (ytN->val[i].Nr < gIpcScadaCfg.yt_num)
                        {
                            gIpcYtData[ytN->val[i].Nr] = ytN->val[i].val;
                            gIpcYtData_flag[ytN->val[i].Nr / 8] |= 0x01 << (ytN->val[i].Nr % 8);
                        }
                        SCADA_log("Nr[%d].val=%f ", ytN->val[i].Nr, ytN->val[i].val);
                    }
                    SCADA_log("\n");
                    pAck->cmd = IPC_SCADA_MSG_CMD_YT_N;
                    pAck->ack = ACK_SUCCED;
                    pAck->num = ytN->num;
                    flag = 1;
                    break;
                case IPC_SCADA_MSG_CMD_YK_1:  // 单点遥控指令
                    yk1 = (MSG_YK_1_t *)recv_data.data;
                    SCADA_log("single yk, yk1->Nr=0x%x val_pos=%d val_inv=%d\n",
                              yk1->Nr, yk1->val_pos, yk1->val_inv);

                    errCode = check_Yk1_Order(yk1, &timerNo);
                    if (errCode != 0)
                    {
                        pAck->ack = ACK_FAILED;
                        pAck->errCode = (uint8_t)errCode;
                        pAck->num = 1;
                        pAck->Nr[0] = yk1->Nr;
                    }
                    else
                    {
                        ofs = yk1->Nr / 8;
                        bitNo = (yk1->Nr % 8) * 2;

                        g_ykTimer[timerNo].pt_nr = yk1->Nr;
                        g_ykTimer[timerNo].ofs = ofs;

                        if (yk1->val_pos == YK_VAL_1_POS)
                        {
                            gIpcYkData[ofs] |= (1 << bitNo);  // 置合位
                            g_ykTimer[timerNo].bit_no = bitNo;
                        }

                        if (yk1->val_pos == YK_VAL_0_POS)
                        {
                            gIpcYkData[ofs] |= (1 << (bitNo + 1));  // 置分位
                            g_ykTimer[timerNo].bit_no = bitNo + 1;
                        }

                        start_ykTimer(&g_ykTimer[timerNo]);

                        // ack
                        pAck->ack = ACK_SUCCED;
                        pAck->errCode = 0;
                        pAck->num = 1;
                        pAck->Nr[0] = yk1->Nr;
                    }

                    pAck->cmd = IPC_SCADA_MSG_CMD_YK_1;

                    pAck->num = 1;
                    flag = 1;
                    break;
                case IPC_SCADA_MSG_CMD_YK_N:  // 批量遥控
                    ykN = (MSG_YK_N_t *)recv_data.data;
                    SCADA_log("bulk yk, ");
                    for (int i = 0; i < ykN->num; ++i)
                    {
                        SCADA_log("ykN->val[%d].val_pos=%d val_inv=%d", i,
                                  ykN->val[ykN->num].val_pos,
                                  ykN->val[ykN->num].val_inv);
                    }
                    pAck->cmd = IPC_SCADA_MSG_CMD_YK_N;
                    pAck->ack = ACK_SUCCED;
                    pAck->num = ykN->num;
                    flag = 1;
                    break;
                default:
                    flag = 0;
                    break;
                }
                if (flag)
                {
                    send_data.sum = 0;
                    for (int i = 0; i < send_data.len; ++i)
                    {
                        send_data.sum += send_data.data[i];
                    }

                    status = RPMessage_send(scada_handle,
                                   remote_core_id,
                                   remote_endpt,
                                   LOCAL_ENDPT_FOR_SCADA_RECV,
                                   &send_data,
                                   sizeof(IPC_SCADA_MSG_t));

                    if (status != IPC_SOK)
                    {
                        Debug_logError("failed with code %d\n", status);
                    }
                }
            }
        }
    }
}

void soe_upload(void)
{
    // 依次检测数组中的每个元素是否发生变化
    for (uint8_t ofs = 0; ofs < IPC_YX_MAX_BUF_SIZE_WORDS; ++ofs)
    {
        if (gIpcYxData[ofs] != gIpcYxDataBak[ofs])
        {
            IPC_SCADA_MSG_t send_data;

            MSG_SOE_t *soe = (MSG_SOE_t *)&send_data.data[0];
            int32_t status;
            uint8_t temp, temp1;

            send_data.dst = IPC_MPU1_0;
            send_data.src = Ipc_mpGetSelfId();
            send_data.typ = IPC_SCADA_MSG_SOE;
            send_data.len = sizeof(MSG_SOE_t);

            APP_UTC_TIME 	UtcTime_src = {0};
            APP_StruTime 	StruTime_dest = {0};

            cache_inv_com(&g_shm_irigb_info.Clk_time_Edge, sizeof(CLK_TIME_EDGE), CacheP_TYPE_ALL);
            UtcTime_src.utc_secs=g_shm_irigb_info.Clk_time_Edge.nUTC;
            UtcTime_src.fraction=g_shm_irigb_info.Clk_time_Edge.nFrc;
            UtcTime_src.uSec_100 = g_shm_irigb_info.Clk_time_Edge.uSec_100_cnt;
            UtcTime_To_StruTime_app(&UtcTime_src,&StruTime_dest, g_shm_irigb_info.Clk_time_Edge.sTimeZone_min);

            soe->timestamp.wYear = StruTime_dest.Year;
            soe->timestamp.byMon = StruTime_dest.Month;
            soe->timestamp.byDay = StruTime_dest.Date;
            soe->timestamp.byHour = StruTime_dest.Hour;
            soe->timestamp.byMin = StruTime_dest.Min;
            soe->timestamp.wMs  = StruTime_dest.uSec_100 * 0.1;

//            soe->timestamp.wYear = 2023;
//            soe->timestamp.byMon = 12;
//            soe->timestamp.byDay = 21;
//            soe->timestamp.byHour = 12;
//            soe->timestamp.byMin = 30;
//            soe->timestamp.wMs = 0;

            soe->num = 0;
            for (uint8_t index = 0; index < 16; ++index)  // 依次检测每个位是否发生变化
            {
                temp = (gIpcYxData[ofs] >> index) & 0x0001;
                temp1 = (gIpcYxDataBak[ofs] >> index) & 0x0001;
                if (temp1 != temp)
                {
                    soe->DI_CHG[soe->num].Nr = (ofs * 16) + index + 1;
                    soe->DI_CHG[soe->num].diSta1 = temp1;
                    soe->DI_CHG[soe->num].diSta2 = temp;
                    soe->num += 1;
                }
            }

            send_data.sum = 0;
            for (int i = 0; i < send_data.len; ++i)
            {
                send_data.sum += send_data.data[i];
            }

            status = RPMessage_send(scada_handle,
                           IPC_MPU1_0,
                           REMOTE_ENDPT_FOR_SCADA_SEND,
                           LOCAL_ENDPT_FOR_SCADA_RECV,
                           &send_data,
                           sizeof(IPC_SCADA_MSG_t));
            if (status != IPC_SOK)
            {
                Debug_logError("failed with code %d\n", status);
            }
            else
            {
                SCADA_log("send yx change\n");
            }

            gIpcYxDataBak[ofs] = gIpcYxData[ofs];
        }
    }
}

#endif