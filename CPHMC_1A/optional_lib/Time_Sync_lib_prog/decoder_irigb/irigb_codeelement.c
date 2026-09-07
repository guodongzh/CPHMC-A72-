

#include "time_handle_main_prog.h"
#include "irigb_codeelement.h"
#include "irigb_decoder.h"

// 功能：解析B码的码元值
// 输入: @param p_Irigb_Fpga_2_cpu_interface B码沿信息,p_Irigb_code_element 当前B码信息
//输出： pHwClockTag [输出]上升沿的硬件时标,更新 p_Irigb_code_element 当前B码信息
//返回 eICE_ZERO/eICE_ONE/eICE_P表示正常;>eICE_P表示解码异常(eICE_BUFFERING表示解码缓冲中)
eIRIGBCodeElement Analysis_Irigb_Codeelement(uint32_t* pHwClockTag_Rise, IRIGB_FPGA_2_CPU_INTERFACE_STRUCT *p_Irigb_Fpga_2_cpu_interface, IRIGB_CODE_ELEMENT_STRUCT* p_Irigb_code_element)
{
uint32_t nPosWidth;
uint32_t nNegWidth;
eIRIGBCodeElement nResult;

	nResult = eICE_BUFFERING;							//正在缓存，还没有捕捉到整个完整的码元，需要收到下降沿后才能判断B码码元
	switch (p_Irigb_code_element->psCounter)							//B码沿接收状态标志 0-起始状态，需要捕作下降沿
	{
	case 0:																//B码接收起始状态，必须为上升沿
		if ((p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity & 0xFF) != ePP_Raise){		//捕捉上升沿，如不是则出错
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=10;
			return eICE_ERROR_FIRST;	// 起始状态捕捉沿出错
		}
		p_Irigb_code_element->tPulseRaise = p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue;				//上升沿的计数值，
		p_Irigb_code_element->psCounter++;																//置B码已收到上升沿标志
		break;
	case 1:														//上次B码为上升沿，还没有捕捉到整个完整的码元
		if ((p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity & 0xFF) != ePP_Fall)
		{
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=11;
			p_Irigb_code_element->psCounter = 0;				// reset
			return eICE_ERROR_SECOND;						// B码已捕捉到起始状态的上升沿，捕捉下降沿时出错
		}
		p_Irigb_code_element->tPulseFall = p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue;			//获得下降沿的计数值
		p_Irigb_code_element->psCounter++;								//置B码已收到下降沿标志
		break;
	case 2:													//上次B码为下降沿
		if ((p_Irigb_Fpga_2_cpu_interface->Irigb_edgePolarity & 0xFF) != ePP_Raise)		//期望的极性出错，期望为下降沿
		{
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
			Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=12;
			p_Irigb_code_element->psCounter = 0;			// reset，重新开始
			return eICE_ERROR_THIRD;		// 第三个沿应该是上升沿
		}
//接收到完整的1个脉冲，解码
		nPosWidth = (uint32_t)(p_Irigb_code_element->tPulseFall - p_Irigb_code_element->tPulseRaise);			//正电平宽度
		nNegWidth = (uint32_t)(p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue - p_Irigb_code_element->tPulseFall);			//当前为上升沿，负电平宽度
		if (((_5ms - MAX_IRIGB_WID_ERR) < nPosWidth)&& (nPosWidth < (_5ms + MAX_IRIGB_WID_ERR))){			// 正脉宽是5ms														//正电平是否为5ms（“1”）在合理范围以内
			if (((_5ms - MAX_IRIGB_WID_ERR) < nNegWidth)&& (nNegWidth < (_5ms + MAX_IRIGB_WID_ERR))){		// 负脉宽是5ms
				nResult = eICE_ONE;												//码元为“1”
			}
			else																//码元出错
			{// 负脉宽错误
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
				Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=13;
				nResult = eICE_ERROR_ONE;
			}
		}
		else							
		{// 正脉宽不是5ms
			if (nPosWidth > _5ms)										//高电平大于5ms
			{// 判断是否是"P"
				if (((_8ms - MAX_IRIGB_WID_ERR) < nPosWidth)			//是否在8ms（P码元）以内
					&& (nPosWidth < (_8ms + MAX_IRIGB_WID_ERR))
					&& ((_2ms - MAX_IRIGB_WID_ERR) < nNegWidth)
					&& (nNegWidth < (_2ms + MAX_IRIGB_WID_ERR)))
				{
					nResult = eICE_P;									//P码元
				}
				else
				{
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=14;
					nResult = eICE_ERROR_P;
				}
			}
			else// (nPosWidth < _5ms)									//小于5ms
			{// 判断是否是"0"
				if (((_2ms - MAX_IRIGB_WID_ERR) < nPosWidth)		//是否在2ms（“0”）以内
					&& (nPosWidth < (_2ms + MAX_IRIGB_WID_ERR))
					&& ((_8ms - MAX_IRIGB_WID_ERR) < nNegWidth)
					&& (nNegWidth < (_8ms + MAX_IRIGB_WID_ERR)))
				{
					nResult = eICE_ZERO;						//"0"
				}
				else
				{
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
					Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=15;
					nResult = eICE_ERROR_ZERO;
				}
			}
		}
		*pHwClockTag_Rise = p_Irigb_code_element->tPulseRaise;									// 只有在上升沿时，输出硬件时标，
		
		p_Irigb_code_element->tPulseRaise = p_Irigb_Fpga_2_cpu_interface->Irigb_CapValue;		//B码上升沿FPGA的计数值，准备下一码元
		p_Irigb_code_element->psCounter = 1;													//置上次B码上升沿标志
		break;
	default:
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_NUMBER]++;
		Share_Ram_time_test_inf.time_test_inf[TEST_INF_UINT32_REC_IRIGB_ERR_LOCATION]=16;
		p_Irigb_code_element->psCounter = 0;						// B码沿重新捕捉第一个码元
		return eICE_Count;
	}
	return nResult;
}

