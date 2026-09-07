#include "nltcp.h"

/*--------------------------------------------------------------------------- */
/* global variables and definitions */
/*--------------------------------------------------------------------------- */
uint8_t gOpenMPC_RecvBuf[5000];
uint8_t gOpenMPC_SendBuf[5000];

PSCBYTE  *gData_p       = PSCNULL;//指向命令接收缓存
PSCDWORD g_dwDataSize   = 0;      //当前接收到的命令的大小
PSCDWORD g_dwDataSize_p = 0;      //命令接收缓存的大小
PSCDWORD gTimeOut       = 0;      //接收超时，设置为0的

PSCBOOL fUsedStaticSendMemory = PSCFALSE;
/********************************************************************/
/* FUNCTION:  PSCPUBLIC   NetSendData                               */
/* DESCRIPT:                                                        */
/* CALLED:                                                          */
/********************************************************************/
PSCBYTE NetSendData(PSCBYTE *pData_p,
                    PSCDWORD dwDataSize_p,
                    PSCBYTE bDataType_p,
                    PSCDWORD dTxTimeOut_p)
{
    PSCBYTE *newBuf = gOpenMPC_SendBuf;
    PSCWORD dwDataSize = dwDataSize_p;
    PSCWORD dwSendData = 0;
    int iSend          = 0;

    if (dTxTimeOut_p != 0)
    {
        /* set the timeout for send */
    }

    if ((dwDataSize + 5) > sizeof(gOpenMPC_SendBuf))
    {
        return NET_ERROR;
    }

    *(PSCDWORD *)newBuf = dwDataSize + 5;//前4个Byte表示：数据大小，+5是把又封装了一层的包头大小计算进去了
    newBuf[4]           = bDataType_p;

    memcpy(&newBuf[5], pData_p, dwDataSize);

    // dwDataSize += 5; //tcp
    dwDataSize += 1; //udp

    /* send may return without all bytes sent.
       here we try to send the rest of the bytes as long errno is EGAIN */
    while (dwSendData < dwDataSize)
    {
        // TODO: Multicore_send
        iSend = NET_PortSend(&newBuf[4] + dwSendData, dwDataSize - dwSendData);
        if (iSend < 0)
        {
            return NET_ERROR;
        }

        dwSendData += iSend;
    }

    return NET_SUCCESS;
}

/********************************************************************/
/* FUNCTION:  PSCPUBLIC   NetRecData                                */
/* DESCRIPT:                                                        */
/* CALLED:                                                          */
/********************************************************************/
PSCBYTE NetRecData(PSCBYTE *pData_p,
                   PSCDWORD dwDataSize_p,
                   PSCBYTE bDataType_p,
                   PSCDWORD dRxTimeOut_p)
{
    gData_p        = pData_p;
    g_dwDataSize_p = dwDataSize_p;
    gTimeOut       = dRxTimeOut_p;

    return NET_SUCCESS;
}

/********************************************************************/
/* FUNCTION:  PSCPUBLIC  NetGetRxStatus                             */
/* DESCRIPT:                                                        */
/* CALLED:                                                          */
/********************************************************************/
PSCBYTE NetGetRxStatus()
{
    uint32_t readSize = 0;
    uint8_t *recv_buf;

    recv_buf = gOpenMPC_RecvBuf;
    readSize = NET_PortRecv();

    if (readSize == 0)
    {
        PSCTRACE("\n -> %s: network is closed", __FUNCTION__);
        return NET_NODATA;
    }
    else
    {
        g_dwDataSize += readSize;

        /* not all data received -> return NET_NODATA then we will be called again */
        if (g_dwDataSize < g_dwDataSize_p && gTimeOut != 0)
        {
            memcpy(gData_p, recv_buf, readSize);
            gData_p += readSize;
            PSCTRACE("\n -> %s: current read size=%d, g_dwDataSize=%d, g_dwDataSize_p=%d recv again",
                     __FUNCTION__, readSize, g_dwDataSize, g_dwDataSize_p);
            return NET_NODATA;
        }
        else
        {
            /* we got all bytes or we do not know how many we need */
            if (g_dwDataSize == g_dwDataSize_p || gTimeOut == 0)
            {
                memcpy(gData_p, recv_buf, readSize);
                gData_p += readSize;
                g_dwDataSize = 0;
                return NET_SUCCESS;
            }
        }
    }

    return NET_NODATA;
}
