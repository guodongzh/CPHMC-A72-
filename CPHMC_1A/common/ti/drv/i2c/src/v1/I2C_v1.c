/**
 *  \file   I2C_v1.c
 *
 *  \brief  IP version 1 specific I2C Driver APIs implementation.
 *
 *   This file contains the driver APIs for I2C controller.
 */

/*
 * Copyright (C) 2014-2017 Texas Instruments Incorporated - http://www.ti.com/
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of Texas Instruments Incorporated nor the names of
 * its contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>
#include <stdbool.h>
#include <ti/drv/i2c/I2C.h>
#include <ti/drv/i2c/soc/I2C_v1.h>
#include <ti/csl/src/ip/i2c/V2/i2c.h>
#include <ti/csl/src/ip/i2c/V2/hw_i2c.h>
#include <ti/drv/i2c/src/I2C_drv_log.h>
#include <ti/csl/hw_types.h>
#include "ti/csl/src/ip/spinlock/V1/csl_spinlock.h"
#include "ti/csl/soc/j721e/src/cslr_soc_baseaddress.h"

#define I2C_MODULE_INTERNAL_CLK_4MHZ   (4000000U)
#define I2C_MODULE_INTERNAL_CLK_12MHZ  (12000000U)
#define I2C_MODULE_INTERNAL_CLK_9P6MHZ (9600000U)

/*
 * Maximum number of loop count to handle in the same ISR, which is
 * to process multiple interrupts (ARDY, RRDY, XRDY) in the ISR to
 * reduce interrupt count
 *
 * Keep at least 3 to support optimal RX followed by TX scenario
 * Keep at least 2 to support optimal RX scenario
 */
#define I2C_MAX_CONSECUTIVE_ISRS       (1U)

/* I2C AM57x functions */
static void I2C_close_v1(I2C_Handle handle);
static void I2C_init_v1(I2C_Handle handle);
static I2C_Handle I2C_open_v1(I2C_Handle handle, const I2C_Params *params);
static int16_t I2C_transfer_v1(I2C_Handle handle, I2C_Transaction *transaction);
static int16_t I2C_primeTransfer_v1(I2C_Handle handle, I2C_Transaction *transaction);
static int32_t I2C_v1_control(I2C_Handle handle, uint32_t cmd, void *arg);
static int32_t I2C_v1_waitForBb(uint32_t baseAddr, uint32_t timeout);
static void I2C_v1_udelay(uint32_t delay);
static int32_t I2C_v1_setBusFrequency(I2C_Handle handle, uint32_t busFrequency);

static int32_t I2C_v1_recoverBus(I2C_Handle handle, uint32_t i2cDelay);
static int32_t I2C_v1_resetCtrl(I2C_Handle handle);
static int32_t I2C_v1_ctrlInit(I2C_Handle handle);
static uint32_t I2C_v1_waitForPin(I2C_Handle handle, uint32_t flag, uint32_t *pTimeout);

#define I2C_DELAY_MED   ((uint32_t)10000U)
#define I2C_DELAY_BIG   ((uint32_t)30000U)
#define I2C_DELAY_SMALL ((uint32_t)5000U)
#define I2C_DELAY_USEC  ((uint32_t)250U)

/* I2C function table for I2C AM57x implementation */
const I2C_FxnTable I2C_v1_FxnTable = {
    &I2C_close_v1,
    &I2C_v1_control,
    &I2C_init_v1,
    &I2C_open_v1,
    &I2C_transfer_v1,
};

static void I2C_lock(I2C_Handle handle)
{
    uint32_t status = CSL_SPINLOCK_VAL_TAKEN;
    I2C_v1_Object *object = (I2C_v1_Object *)handle->object;

    /* Spin till lock is acquired */
    if (object->i2cParams.lockNumber != (uint32_t)I2C_INVALID_LOCK_NUMBER)
    {
        while (1U)
        {
            status = SPINLOCKLockStatusSet(CSL_NAVSS0_SPINLOCK_BASE, object->i2cParams.lockNumber);
            if (status == CSL_SPINLOCK_VAL_FREE)
            {
                break; /* Free and taken */
            }
        }
    }
}

static void I2C_unlock(I2C_Handle handle)
{
    I2C_v1_Object *object = (I2C_v1_Object *)handle->object;
    if (object->i2cParams.lockNumber != (uint32_t)I2C_INVALID_LOCK_NUMBER)
    {
        SPINLOCKLockStatusFree(CSL_NAVSS0_SPINLOCK_BASE, object->i2cParams.lockNumber);
    }
}

/*
 *  ======== I2C_close_v1 ========
 */
static void I2C_close_v1(I2C_Handle handle)
{
    I2C_v1_Object *object = NULL;
    I2C_HwAttrs const *hwAttrs = NULL;

    /* Get the pointer to the object and hwAttrs */
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;
    object = (I2C_v1_Object *)handle->object;

    /* Check to see if a I2C transaction is in progress */
    if (NULL == object->headPtr)
    {
        I2C_lock(handle);
        /* Mask I2C interrupts */
        I2CMasterIntDisableEx(hwAttrs->baseAddr, I2C_INT_ALL);

        /* Disable the I2C Master */
        I2CMasterDisable(hwAttrs->baseAddr);

        I2C_unlock(handle);
        object->isOpen = BFALSE;

        I2C_drv_log1("\n I2C: Object closed 0x%x \n", hwAttrs->baseAddr);
    }
}
/*
 *  ======== I2C_init_v1 ========
 */
static void I2C_init_v1(I2C_Handle handle)
{
    /* Input parameter validation */
    if (NULL != handle)
    {
        /* Mark the object as available */
        ((I2C_v1_Object *)(handle->object))->isOpen = BFALSE;
    }
}

/*
 *  ======== I2C_open_v1 ========
 */
static I2C_Handle I2C_open_v1(I2C_Handle handle, const I2C_Params *params)
{
    uint32_t outputClk;
    I2C_v1_Object *object = NULL;
    I2C_HwAttrs const *hwAttrs = NULL;
    uint32_t internalClk;
    I2C_Handle retHandle = handle;

    /* Get the pointer to the object and hwAttrs */
    object = (I2C_v1_Object *)handle->object;
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    /* Determine if the device index was already opened */
    if (BTRUE == object->isOpen)
    {
        retHandle = NULL;
    }
    else
    {
        /* Mark the handle as being used */
        object->isOpen = BTRUE;

        /* Copy the params contents */
        object->i2cParams = *params;

        /* Specify the idle state for this I2C peripheral */
        object->mode = I2C_IDLE_MODE;

        /* Clear the head pointer */
        object->headPtr = NULL;
        object->tailPtr = NULL;

#ifdef BUILD_MCU1_0

        I2C_lock(handle);
        I2C_drv_log1("\n I2C: Object created 0x%x \n", hwAttrs->baseAddr);
        /* Put i2c in reset/disabled state */
        I2CMasterDisable(hwAttrs->baseAddr);

        /* Disable Auto Idle functionality */
        I2CAutoIdleDisable(hwAttrs->baseAddr);

        /* Extract bit rate from the input parameter */
        switch (object->i2cParams.bitRate)
        {
        case I2C_100kHz: {
            outputClk = 100000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
            break;
        }

        case I2C_400kHz: {
            /* For 400KHz Bus Frequency:
             * I2C Functional Clock: 96MHz(Fixed).
             * TRM recommended Internal Clock: 9.6MHz.
             *   => i.e., 96MHz is divided by a prescalar of 10.
             * To get a bus frequence of 400KHz,
             * 9.6MHz internal clock is divided internally by 24(9.6MHz/24 = 400KHz).
             * Based on the InternalCLk and outputClk, CSL calculates the internal divider.
             */
            outputClk = 400000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_9P6MHZ;
            break;
        }

        case I2C_1P0Mhz: {
            outputClk = 1000000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_12MHZ;
            break;
        }

        case I2C_3P4Mhz: {
            outputClk = 3400000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_12MHZ;
            break;
        }

        default: {
            /* Default case force it to 100 KHz bit rate */
            outputClk = 100000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
        }
        break;
        }

        /* Set the I2C configuration */
        I2CMasterInitExpClk(hwAttrs->baseAddr, hwAttrs->funcClk, internalClk, outputClk);

        /* attention */
        /* Clear any pending interrupts */
        I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

        /* Mask off all interrupts */
        I2CMasterIntDisableEx(hwAttrs->baseAddr, I2C_INT_ALL);

        /* Enable the I2C Master for operation */
        I2CMasterEnable(hwAttrs->baseAddr);

        /* Enable free run mode */
        I2CMasterEnableFreeRun(hwAttrs->baseAddr);

        I2C_unlock(handle);
#endif
        /* Return the address of the i2cObjectArray[i] configuration struct */
    }
    return retHandle;
}

static bool I2C_checkTimeout(uint32_t *pUsecCnt)
{
    bool timeout = BFALSE;

    *pUsecCnt = *pUsecCnt + 1U;
    if (1000U == *pUsecCnt)
    {
        *pUsecCnt = 0U;
        timeout = BTRUE;
    }

    return timeout;
}

/*
 *  ======== I2C_primeTransfer_v1 =======
 */
static int16_t I2C_primeTransfer_v1(I2C_Handle handle, I2C_Transaction *transaction)
{
    I2C_v1_Object *object = NULL;
    I2C_HwAttrs const *hwAttrs = NULL;
    int16_t status = I2C_STS_SUCCESS;
    uint32_t errStat = UFALSE, fatalError = UFALSE;
    uint32_t regVal;
    uint32_t xsa;
    uint32_t timeout = transaction->timeout;
    uint32_t uSecTimeout = 0U;

    /* Get the pointer to the object and hwAttrs */
    object = (I2C_v1_Object *)handle->object;
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    /* Store the new internal counters and pointers */
    object->currentTransaction = transaction;
    object->writeBufIdx = (uint8_t *)transaction->writeBuf;
    object->writeCountIdx = (uint32_t)transaction->writeCount;
    object->readBufIdx = (uint8_t *)transaction->readBuf;
    object->readCountIdx = (uint32_t)transaction->readCount;
    object->intStatusErr = 0U;
    object->mode = I2C_IDLE_MODE;

    I2C_drv_log2("\n I2C:(0x%x) Starting transaction to slave: 0x%x \n",
                 hwAttrs->baseAddr,
                 object->currentTransaction->slaveAddress);

    /* clear all interrupts */
    I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

    if (transaction->masterMode)
    {
        if (BTRUE == object->currentTransaction->expandSA)
        {
            /* enable the 10-bit address mode */
            xsa = I2C_CFG_10BIT_SLAVE_ADDR;
        }
        else
        {
            /* enable the 7-bit address mode */
            xsa = I2C_CFG_7BIT_SLAVE_ADDR;
        }

        /* In master mode, set the I2C slave address */
        I2CMasterSlaveAddrSet(hwAttrs->baseAddr, object->currentTransaction->slaveAddress);

        /* POLLING MODE */
        if (0U != object->writeCountIdx)
        {
            /* set number of bytes to write */
            I2CSetDataCount(hwAttrs->baseAddr, object->writeCountIdx);

            /* set to master transmitter mode */
            regVal = I2C_CFG_MST_TX | xsa;
            if ((I2C_1P0Mhz == object->i2cParams.bitRate) ||
                (I2C_3P4Mhz == object->i2cParams.bitRate))
            {
                regVal |= I2C_CFG_HS_MOD;
            }

            /* wait for bus busy */
            while ((0U != timeout) &&
                   (BTRUE == I2CMasterBusBusy(hwAttrs->baseAddr))) /* address mask */
            {
                I2C_v1_udelay(I2C_DELAY_USEC);
                if (I2C_checkTimeout(&uSecTimeout))
                {
                    timeout--;
                }
            }

            I2CMasterControl(hwAttrs->baseAddr, regVal);

            /* generate start */
            I2CMasterStart(hwAttrs->baseAddr);

            while ((0U != object->writeCountIdx) && (0U != timeout))
            {
                /* wait for transmit ready or error */
                while (((0U == I2CMasterIntRawStatusEx(hwAttrs->baseAddr, I2C_INT_TRANSMIT_READY)) &&
                        (0U == I2CMasterIntRawStatusEx(hwAttrs->baseAddr,I2C_INT_ARBITRATION_LOST |
                                                                              I2C_INT_NO_ACK |
                                                                              I2C_INT_ACCESS_ERROR |
                                                                              I2C_INT_STOP_CONDITION))) &&
                    (0U != timeout))
                {
                    I2C_v1_udelay(I2C_DELAY_USEC);
                    if (I2C_checkTimeout(&uSecTimeout))
                    {
                        timeout--;
                    }
                }

                errStat = I2CMasterIntRawStatusEx(hwAttrs->baseAddr,I2C_INT_ARBITRATION_LOST |
                                                                         I2C_INT_NO_ACK |
                                                                         I2C_INT_ACCESS_ERROR);

                /* if we get an error, do a stop and return failure */
                if (UFALSE != errStat)
                /* if we get an error, do a stop and return failure */
                {
                    fatalError = UTRUE;
                    break;
                }
                /* write byte and increase data pointer to next byte */
                I2CMasterDataPut(hwAttrs->baseAddr, *(object->writeBufIdx));
                (object->writeBufIdx)++;

                /* clear transmit ready interrupt */
                I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_TRANSMIT_READY);

                /* update number of bytes written */
                object->writeCountIdx--;
            }

            if ((UFALSE == fatalError) && (0U != timeout))
            {
                /* wait for register access ready */
                timeout = I2C_v1_waitForPin(handle, I2C_INT_ADRR_READY_ACESS, &timeout);
                /* Read status again to make sure there are no errors
                 * after register access is available and data is written */
                errStat = I2CMasterIntRawStatusEx(hwAttrs->baseAddr,
                                                  I2C_INT_ARBITRATION_LOST | I2C_INT_NO_ACK |
                                                      I2C_INT_ACCESS_ERROR);
            }

            if (I2C_INT_ARBITRATION_LOST == (errStat & I2C_INT_ARBITRATION_LOST))
            {
                status = I2C_STS_ERR_ARBITRATION_LOST;
            }
            else if (I2C_INT_NO_ACK == (errStat & I2C_INT_NO_ACK))
            {
                status = I2C_STS_ERR_NO_ACK;
            }
            else if (I2C_INT_ACCESS_ERROR == (errStat & I2C_INT_ACCESS_ERROR))
            {
                status = I2C_STS_ERR_ACCESS_ERROR;
            }
            else if (0U == timeout)
            {
                status = I2C_STS_ERR_TIMEOUT;
            }
            else
            {
                status = I2C_STS_SUCCESS;
            }

            if (0U == object->readCountIdx)
            {
                /* generate stop when there is no read following by write */
                I2CMasterStop(hwAttrs->baseAddr);

                if ((UFALSE == fatalError) && (0U != timeout))
                {
                    /* wait for stop to happen */
                    timeout = I2C_v1_waitForPin(handle, I2C_INT_STOP_CONDITION, &timeout);

                    /* wait for register access ready */
                    timeout = I2C_v1_waitForPin(handle, I2C_INT_ADRR_READY_ACESS, &timeout);

                    if (0U == timeout)
                    {
                        status = I2C_STS_ERR_TIMEOUT;
                    }
                }
            }
        }

        if ((0U != object->readCountIdx) && (I2C_STS_SUCCESS == status))
        {
            /* clear all interrupts */
            I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

            /* set number of bytes to read */
            I2CSetDataCount(hwAttrs->baseAddr, object->readCountIdx);

            /* set to master receiver mode */
            regVal = I2C_CFG_MST_RX | xsa;
            if ((I2C_1P0Mhz == object->i2cParams.bitRate) ||
                (I2C_3P4Mhz == object->i2cParams.bitRate))
            {
                regVal |= I2C_CFG_HS_MOD;
            }

            /* wait for bus not busy.
             * Check bus busy for read-only transfers to support
             * repeat start condition during write address and read data.
             */
            if (NULL == object->writeBufIdx)
            {
                while ((BTRUE == I2CMasterBusBusy(hwAttrs->baseAddr)) && (0U != timeout))
                {
                    I2C_v1_udelay(I2C_DELAY_USEC);
                    if (I2C_checkTimeout(&uSecTimeout))
                    {
                        timeout--;
                    }
                }
            }

            I2CMasterControl(hwAttrs->baseAddr, regVal);

            /* generate start */
            I2CMasterStart(hwAttrs->baseAddr);

            while ((0U != object->readCountIdx) && (0U != timeout))
            {
                /* wait for receive ready or error */
                while (((0U == I2CMasterIntRawStatusEx(hwAttrs->baseAddr, I2C_INT_RECV_READY)) &&
                        (0U == I2CMasterIntRawStatusEx(hwAttrs->baseAddr,
                                                       I2C_INT_ARBITRATION_LOST | I2C_INT_NO_ACK |
                                                           I2C_INT_ACCESS_ERROR))) &&
                       (0U != timeout))
                {
                    I2C_v1_udelay(I2C_DELAY_USEC);
                    if (I2C_checkTimeout(&uSecTimeout))
                    {
                        timeout--;
                    }
                }

                errStat = I2CMasterIntRawStatusEx(hwAttrs->baseAddr,
                                                  I2C_INT_ARBITRATION_LOST | I2C_INT_NO_ACK |
                                                      I2C_INT_ACCESS_ERROR);

                /* if we get an error, do a stop and return failure */
                if (UFALSE != errStat)
                {
                    fatalError = UTRUE;
                    break;
                }

                /* read byte and increase data pointer to next byte */
                *(object->readBufIdx) = (uint8_t)I2CMasterDataGet(hwAttrs->baseAddr);

                /* clear receive ready interrupt */
                I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_RECV_READY);

                object->readBufIdx++;
                object->readCountIdx--; /* update number of bytes read */
            }

            if ((UFALSE == fatalError) && (0U != timeout))
            {
                /* wait for register access ready */
                timeout = I2C_v1_waitForPin(handle, I2C_INT_ADRR_READY_ACESS, &timeout);
                /* Read status again to make sure there are no errors
                 * after register access is available and data is read */
                errStat = I2CMasterIntRawStatusEx(hwAttrs->baseAddr,
                                                  I2C_INT_ARBITRATION_LOST | I2C_INT_NO_ACK |
                                                      I2C_INT_ACCESS_ERROR);
            }

            if (I2C_INT_ARBITRATION_LOST == (errStat & I2C_INT_ARBITRATION_LOST))
            {
                status = I2C_STS_ERR_ARBITRATION_LOST;
            }
            else if (I2C_INT_NO_ACK == (errStat & I2C_INT_NO_ACK))
            {
                status = I2C_STS_ERR_NO_ACK;
            }
            else if (I2C_INT_ACCESS_ERROR == (errStat & I2C_INT_ACCESS_ERROR))
            {
                status = I2C_STS_ERR_ACCESS_ERROR;
            }
            else if (0U == timeout)
            {
                status = I2C_STS_ERR_TIMEOUT;
            }
            else
            {
                status = I2C_STS_SUCCESS;
            }

            /* generate stop when requested */
            I2CMasterStop(hwAttrs->baseAddr);

            if ((UFALSE == fatalError) && (0U != timeout))
            {
                /* wait for stop to happen */
                timeout = I2C_v1_waitForPin(handle, I2C_INT_STOP_CONDITION, &timeout);

                /* wait for register access ready */
                timeout = I2C_v1_waitForPin(handle, I2C_INT_ADRR_READY_ACESS, &timeout);

                if (0U == timeout)
                {
                    status = I2C_STS_ERR_TIMEOUT;
                }
            }
        }
    }
#if (1U == I2C_ENABLE_SLAVE_MODE)
    /* In slave mode */
    else
    {
        if (BTRUE == object->currentTransaction->expandSA)
        {
            /* enable the 10-bit address mode */
            xsa = I2C_CFG_10BIT_OWN_ADDR_0;
        }
        else
        {
            /* enable the 7-bit address mode */
            xsa = I2C_CFG_7BIT_OWN_ADDR_0;
        }

        /* Currently slave mode is supported only when interrupt is enabled */
        if (I2C_OPER_MODE_POLLING != object->operMode)
        {
            /* In slave mode, set the I2C own address */
            I2COwnAddressSet(hwAttrs->baseAddr, hwAttrs->ownSlaveAddr[0], I2C_OWN_ADDR_0);

            /* Configure data buffer length to 0 as the actual number of bytes to
               transmit/receive is dependant on external master. */
            I2CSetDataCount(hwAttrs->baseAddr, 0U);

            /* Enable interrupts in slave mode */
            I2CSlaveIntEnableEx(hwAttrs->baseAddr,
                                I2C_INT_TRANSMIT_READY | I2C_INT_RECV_READY |
                                    I2C_INT_ADRR_READY_ACESS | I2C_INT_ADRR_SLAVE);

            /* Start the I2C transfer in slave mode */
            regVal = I2C_CFG_MST_ENABLE | xsa;
            if ((I2C_1P0Mhz == object->i2cParams.bitRate) ||
                (I2C_3P4Mhz == object->i2cParams.bitRate))
            {
                regVal |= I2C_CFG_HS_MOD;
            }
            I2CMasterControl(hwAttrs->baseAddr, regVal);
        }
    }
#endif
    return status;
}

/*
 *  ======== I2C_transfer_v1 ========
 */
static int16_t I2C_transfer_v1(I2C_Handle handle, I2C_Transaction *transaction)
{
    int16_t retVal = I2C_STS_ERR;
    bool ret_flag = BFALSE;
    I2C_HwAttrs const *hwAttrs = NULL;

    if ((NULL != handle) && (NULL != transaction))
    {
        hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

        if (0U == (transaction->validParams & I2C_TRANS_VALID_PARAM_MASTER_MODE))
        {
            /*
             * masterMode valid param bit field is not set,
             * set masterMode to default mode (master)
             */
            transaction->masterMode = BTRUE;
        }

        if (0U == (transaction->validParams & I2C_TRANS_VALID_PARAM_EXPAND_SA))
        {
            /*
             * expandSA valid param bit field is not set,
             * set to 7-bit address mode by default
             */
            transaction->expandSA = BFALSE;
        }

        if (0U == transaction->timeout)
        {
            /* timeout cannot be NO_WAIT, set it to default value */
            transaction->timeout = I2C_WAIT_FOREVER;
        }
    }
    else
    {
        ret_flag = BTRUE;
    }

    if ((BFALSE == ret_flag) && ((0U != transaction->writeCount) || (0U != transaction->readCount)))
    {
        I2C_lock(handle);
        /*
         * Clear the RX + TX FIFOs. If the previous transfer failed due to an error,
         * there's a possibility data could still be in the FIFO.
         */
        I2CFIFOClear(hwAttrs->baseAddr, I2C_TX_MODE);
        I2CFIFOClear(hwAttrs->baseAddr, I2C_RX_MODE);
        /*
         * I2CSubArtic_primeTransfer is a longer process and
         * protection is needed from the I2C interrupt
         */

        retVal = I2C_primeTransfer_v1(handle, transaction);

        if (retVal == I2C_STS_SUCCESS)
        {
            /* Polling mode: Wait for the transfer to complete */
            while (I2CMasterBusy(hwAttrs->baseAddr))
            {
                /* Optionally, implement a timeout to avoid infinite loop */
                if (--transaction->timeout == 0U)
                {
                    retVal = I2C_STS_ERR_TIMEOUT;
                    (void)I2C_v1_recoverBus(handle, I2C_DELAY_SMALL);
                    break;
                }
            }

            /* Check for errors in the transfer */
            if (retVal != I2C_STS_ERR_TIMEOUT)
            {
                uint32_t intStatus = I2CMasterErr(hwAttrs->baseAddr);
                if (intStatus & I2C_INT_ARBITRATION_LOST)
                {
                    retVal = I2C_STS_ERR_ARBITRATION_LOST;
                }
                else if (intStatus & I2C_INT_NO_ACK)
                {
                    retVal = I2C_STS_ERR_NO_ACK;
                }
                else if (intStatus & I2C_INT_ACCESS_ERROR)
                {
                    retVal = I2C_STS_ERR_ACCESS_ERROR;
                }
                else
                {
                    retVal = I2C_STS_SUCCESS;
                }
            }
        }
        I2C_unlock(handle);
    }

    return (retVal);
}

/*
 *  ======== I2C_v1_control ========
 */
/*!
 *  @brief      A function pointer to a driver specific implementation of
 *              I2C_control().
 */
static int32_t I2C_v1_control(I2C_Handle handle, uint32_t cmd, void *arg)
{
    int32_t retVal = I2C_STATUS_ERROR;
    uint32_t regVal;
    I2C_HwAttrs const *hwAttrs = NULL;
    I2C_v1_Object *object = NULL;

    /* Get the pointer to hwAttrs */
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;
    object = (I2C_v1_Object *)handle->object;

    /* Acquire the lock for this particular I2C handle */
    I2C_lock(handle);
    switch (cmd)
    {
    case I2C_CMD_PROBE: {
        uint32_t slaveAddr = *((uint32_t *)arg);

        /* Disable interrupts first */
        regVal = I2CGetEnabledIntStatus(hwAttrs->baseAddr, I2C_INT_ALL);

        I2CMasterIntDisableEx(hwAttrs->baseAddr, I2C_INT_ALL);

        /* wait until bus not busy */
        if (I2C_STATUS_SUCCESS == I2C_v1_waitForBb(hwAttrs->baseAddr, I2C_DELAY_MED))
        {
            /* set slave address */
            I2CMasterSlaveAddrSet(hwAttrs->baseAddr, (uint32_t)slaveAddr);

            /* try to write one byte */
            I2CMasterDataPut(hwAttrs->baseAddr, (uint8_t)0U);
            I2CSetDataCount(hwAttrs->baseAddr, (uint32_t)1U);

            /* stop bit needed here */
            I2CConfig(hwAttrs->baseAddr,
                      (I2C_CFG_MST_ENABLE | I2C_CFG_MST_TX | I2C_CFG_START | I2C_CFG_STOP));

            /* enough delay for the NACK bit set */
            I2C_v1_udelay(I2C_DELAY_BIG);

            if (0U == I2CMasterIntRawStatusEx(hwAttrs->baseAddr, I2C_INT_NO_ACK))
            {
                retVal = I2C_STATUS_SUCCESS; /* success case */
            }
            else
            {
                /* Clear sources*/
                I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

                /* finish up xfer */
                I2CMasterStop(hwAttrs->baseAddr);
                (void)I2C_v1_waitForBb(hwAttrs->baseAddr, I2C_DELAY_MED);

                retVal = I2C_STATUS_ERROR; /* Error case */
            }

            I2CFIFOClear(hwAttrs->baseAddr, I2C_TX_MODE);
            I2CFIFOClear(hwAttrs->baseAddr, I2C_RX_MODE);
            I2CSetDataCount(hwAttrs->baseAddr, 0);
            I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);
        }
        else
        {
            retVal = I2C_STATUS_ERROR;
        }

        /* Enable interrupts now */
        I2CMasterIntEnableEx(hwAttrs->baseAddr, regVal);

        break;
    }

    case I2C_CMD_SET_BUS_FREQUENCY: {
        uint32_t busFrequency = *((uint32_t *)arg);

        /* Set the required bus frequency */
        retVal = I2C_v1_setBusFrequency(handle, busFrequency);
        break;
    }

    case I2C_CMD_RECOVER_BUS: {
        uint32_t i2cDelay = *((uint32_t *)arg);

        /* perform Bus recovery */
        retVal = I2C_v1_recoverBus(handle, i2cDelay);
        break;
    }

    default: {
        retVal = I2C_STATUS_UNDEFINEDCMD;
        break;
    }
    }
    /* Release the lock for this particular I2C handle */
    I2C_unlock(handle);
    return retVal;
}

static int32_t I2C_v1_waitForBb(uint32_t baseAddr, uint32_t timeout)
{
    uint32_t stat;
    int32_t retVal = I2C_STATUS_SUCCESS;
    volatile uint32_t bbtimeout = timeout;

    if (0U < bbtimeout)
    {
        /* Clear current interrupts...*/
        I2CMasterIntClearEx(baseAddr, I2C_INT_ALL);

        while (0U < bbtimeout)
        {
            stat = I2CMasterIntRawStatusEx(baseAddr, I2C_INT_BUS_BUSY);
            if (0U == stat)
            {
                break;
            }
            bbtimeout = bbtimeout - 1U;
            I2CMasterIntClearEx(baseAddr, stat);
        }

        if (0U == bbtimeout)
        {
            retVal = I2C_STATUS_ERROR;
        }

        /* clear delayed stuff*/
        I2CMasterIntClearEx(baseAddr, I2C_INT_ALL);
    }
    else
    {
        while (BTRUE == I2CMasterBusBusy(baseAddr))
        {
        }
    }

    return retVal;
}

static void I2C_v1_udelay(uint32_t delay)
{
    volatile uint32_t del = delay;

    while (0U != del)
    {
        del = del - 1U;
    }
}

static int32_t I2C_v1_setBusFrequency(I2C_Handle handle, uint32_t busFrequency)
{
    int32_t retVal = I2C_STATUS_SUCCESS;
    I2C_HwAttrs const *hwAttrs = NULL;
    uint32_t outputClk = 0U;
    uint32_t internalClk = 0U;

    /* Get the pointer to the object and hwAttrs */
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    /* Put i2c in reset/disabled state */
    I2CMasterDisable(hwAttrs->baseAddr);

    /* Extract bit rate from the input parameter */
    switch (busFrequency)
    {
    case (uint32_t)I2C_100kHz: {
        outputClk = 100000U;
        internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
        break;
    }

    case (uint32_t)I2C_400kHz: {
        /* For 400KHz Bus Frequency:
         * I2C Functional Clock: 96MHz(Fixed).
         * TRM recommended Internal Clock: 9.6MHz.
         *   => i.e., 96MHz is divided by a prescalar of 10.
         * To get a bus frequence of 400KHz,
         * 9.6MHz internal clock is divided internally by 24(9.6MHz/24 = 400KHz).
         * Based on the InternalCLk and outputClk, CSL calculates the internal divider.
         */
        outputClk = 400000U;
        internalClk = I2C_MODULE_INTERNAL_CLK_9P6MHZ;
        break;
    }
    default: {
        outputClk = 100000U;
        internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
        break;
    }
    }

    /* Set the I2C configuration */
    I2CMasterInitExpClk(hwAttrs->baseAddr, hwAttrs->funcClk, internalClk, outputClk);

    /* Clear any pending interrupts */
    I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

    /* Mask off all interrupts */
    I2CMasterIntDisableEx(hwAttrs->baseAddr, I2C_INT_ALL);

    /* Enable the I2C Master for operation */
    I2CMasterEnable(hwAttrs->baseAddr);

    /* Enable free run mode */
    I2CMasterEnableFreeRun(hwAttrs->baseAddr);

    retVal = I2C_STATUS_SUCCESS;

    return retVal;
}

/*
 *  ======== I2C_v1_recoverBus ========
 */
static int32_t I2C_v1_recoverBus(I2C_Handle handle, uint32_t i2cDelay)
{
    I2C_HwAttrs const *hwAttrs = NULL;
    I2C_v1_Object *object = NULL;
    int32_t status = I2C_STATUS_ERROR;
    uint32_t sysTest, i;

    /* Get the pointer to hwAttrs */
    object = (I2C_v1_Object *)handle->object;
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    if ((NULL != object) && (NULL != hwAttrs))
    {
        status = I2C_STATUS_SUCCESS;

        /* Check if SDA or SCL is stuck low based on the SYSTEST.
         * If SCL is stuck low we reset the IP.
         * If SDA is stuck low drive 9 clock pulses on SCL and check if the
         * slave has released the SDA. If not we reset the I2C controller.
         */

        sysTest = I2CMasterGetSysTest(hwAttrs->baseAddr);
        if (0U == (sysTest & I2C_SYSTEST_SCL_I_FUNC_MASK))
        {
            /* SCL is stuck low reset the I2C IP */
            status = I2C_v1_resetCtrl(handle);
        }
        else if (0U == (sysTest & I2C_SYSTEST_SDA_I_FUNC_MASK))
        {
            /* SDA is stuck low generate 9 clk pulses on SCL */
            /* switch to system test mode */
            HW_SET_FIELD32(sysTest, I2C_SYSTEST_ST_EN, I2C_SYSTEST_ST_EN_ENABLE);
            HW_SET_FIELD32(sysTest, I2C_SYSTEST_TMODE, I2C_SYSTEST_TMODE_LOOPBACK);
            I2CMasterSetSysTest(hwAttrs->baseAddr, sysTest);
            for (i = 0U; i < 9U; i++)
            {
                HW_SET_FIELD32(sysTest, I2C_SYSTEST_SCL_O, I2C_SYSTEST_SCL_O_SCLOH);
                I2CMasterSetSysTest(hwAttrs->baseAddr, sysTest);
                I2C_v1_udelay(i2cDelay);
                HW_SET_FIELD32(sysTest, I2C_SYSTEST_SCL_O, I2C_SYSTEST_SCL_O_SCLOL);
                I2CMasterSetSysTest(hwAttrs->baseAddr, sysTest);
                I2C_v1_udelay(i2cDelay);
            }
            /* Switch back to functional mode */
            HW_SET_FIELD32(sysTest, I2C_SYSTEST_ST_EN, I2C_SYSTEST_ST_EN_DISABLE);
            HW_SET_FIELD32(sysTest, I2C_SYSTEST_TMODE, I2C_SYSTEST_TMODE_FUNCTIONAL);
            I2CMasterSetSysTest(hwAttrs->baseAddr, sysTest);
            /* Now check if the SDA is releases. If its still stuck low,
             * There is nothing that can be done. We still try to reset our IP.
             */
            sysTest = I2CMasterGetSysTest(hwAttrs->baseAddr);
            if (0U == (sysTest & I2C_SYSTEST_SDA_I_FUNC_MASK))
            {
                status = I2C_v1_resetCtrl(handle);
            }
        }
        else
        {
            /* Nothing to be done. SCA and SDA both are not stuck to low */
        }
    }
    return status;
}

/*
 *  ======== I2C_v1_resetCtrl ========
 */
static int32_t I2C_v1_resetCtrl(I2C_Handle handle)
{
    I2C_HwAttrs const *hwAttrs = NULL;
    I2C_v1_Object *object = NULL;
    int32_t status = I2C_STATUS_ERROR;

    /* Get the pointer to hwAttrs */
    object = (I2C_v1_Object *)handle->object;
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;
    if ((NULL != object) && (NULL != hwAttrs))
    {
        status = I2C_v1_ctrlInit(handle);
    }

    return status;
}

/*
 *  ======== I2C_v1_ctrlInit ========
 */
static int32_t I2C_v1_ctrlInit(I2C_Handle handle)
{
    I2C_HwAttrs const *hwAttrs = NULL;
    I2C_v1_Object *object = NULL;
    uint32_t delay = 50U;
    uint32_t outputClk;
    uint32_t internalClk;
    uint32_t regVal;
    int32_t retVal = I2C_STATUS_ERROR;

    /* Get the pointer to hwAttrs */
    object = (I2C_v1_Object *)handle->object;
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    /* Do a software reset */
    I2CSoftReset(hwAttrs->baseAddr);

    /* Enable i2c module */
    I2CMasterEnable(hwAttrs->baseAddr);

    /* Wait for the reset to get complete  -- constant delay - 50ms */
    while ((0U != delay) && (0U == I2CSystemStatusGet(hwAttrs->baseAddr)))
    {
        delay--;
        I2C_v1_udelay(I2C_DELAY_SMALL);
    }

    if (0U != delay)
    {
        /* Put i2c in reset/disabled state */
        I2CMasterDisable(hwAttrs->baseAddr);

        /* Configure i2c bus speed*/
        switch (object->i2cParams.bitRate)
        {
        case I2C_100kHz: {
            outputClk = 100000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
            break;
        }

        case I2C_400kHz: {
            /* For 400KHz Bus Frequency:
             * I2C Functional Clock: 96MHz(Fixed).
             * TRM recommended Internal Clock: 9.6MHz.
             *   => i.e., 96MHz is divided by a prescalar of 10.
             * To get a bus frequence of 400KHz,
             * 9.6MHz internal clock is divided internally by 24(9.6MHz/24 = 400KHz).
             * Based on the InternalCLk and outputClk, CSL calculates the internal divider.
             */
            outputClk = 400000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_9P6MHZ;
            break;
        }

        case I2C_1P0Mhz: {
            outputClk = 3400000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_12MHZ;
            break;
        }

        default: {
            /* Default case force it to 100 KHz bit rate */
            outputClk = 100000U;
            internalClk = I2C_MODULE_INTERNAL_CLK_4MHZ;
        }
        break;
        }

        /* Set the I2C configuration */
        I2CMasterInitExpClk(hwAttrs->baseAddr, hwAttrs->funcClk, internalClk, outputClk);

        /**
         * Configure I2C_SYSC params
         * Disable auto idle mode
         * Both OCP and systen clock cut off
         * Wake up mechanism disabled
         * No idle mode selected
         */
        regVal =
            I2C_AUTOIDLE_DISABLE | I2C_CUT_OFF_BOTH_CLK | I2C_ENAWAKEUP_DISABLE | I2C_NO_IDLE_MODE;
        I2CSyscInit(hwAttrs->baseAddr, regVal);

        /* Configure I2C_CON params */
        regVal = I2C_OPMODE_FAST_STAND_MODE | I2C_NORMAL_MODE;
        I2CConfig(hwAttrs->baseAddr, regVal);

        /* Take the I2C module out of reset: */
        I2CMasterEnable(hwAttrs->baseAddr);

        /* Enable free run mode */
        I2CMasterEnableFreeRun(hwAttrs->baseAddr);

        retVal = I2C_STATUS_SUCCESS;
    }

    /*Clear status register */
    I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);

    return retVal;
}

/*
 *  ======== I2C_v1_waitForPin ========
 */
static uint32_t I2C_v1_waitForPin(I2C_Handle handle, uint32_t flag, uint32_t *pTimeout)
{
    uint32_t status;
    I2C_HwAttrs const *hwAttrs = NULL;
    uint32_t timeout = *pTimeout;
    uint32_t uSecTimeout = 0U;

    /* Get the pointer to hwAttrs */
    hwAttrs = (I2C_HwAttrs const *)handle->hwAttrs;

    if (0U < timeout)
    {
        status = I2CMasterIntRawStatus(hwAttrs->baseAddr);
        while ((uint32_t)0U == (status & flag))
        {
            if ((uint32_t)0U != timeout)
            {
                I2C_v1_udelay(I2C_DELAY_USEC);
                if (I2C_checkTimeout(&uSecTimeout))
                {
                    timeout--;
                }
                status = I2CMasterIntRawStatus(hwAttrs->baseAddr);
            }
            else
            {
                break;
            }
        }

        if (0U == timeout)
        {
            I2CMasterIntClearEx(hwAttrs->baseAddr, I2C_INT_ALL);
        }
    }

    return (timeout);
}
