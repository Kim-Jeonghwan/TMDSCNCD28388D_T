/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : DevCan.c
    Description      : CAN Device Driver (CANA, GPIO70/71, 1Mbps, Extended Frame, ISR)
    Last Updated     : 2026. 04. 17.
**********************************************************************/

// FLASH 메모리에서 코드 실행 시, 램 영역으로 복사해서 고속 실행하도록 함
#ifdef _FLASH
	#pragma CODE_SECTION(CanaISR, ".TI.ramfunc");
#endif


/* ************************** [[   include  ]]  *********************************************************** */
#include "DevCan.h"

/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */
Uint16      errorFlag;
Uint16      rxMsgData1[MSG_DATA_LENGTH];
extern bool bCanRxFlag;     // CSU_CAN.c에서 관리하는 수신 완료 플래그

/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion	void InitialCan(void)
@brief		
@param		void
@return		void
@remark	
	-	
*/
void InitialCan(void)
{
	InitialCana();
	//InitialCanb();	// CAN B를 같이 사용할 경우 
}



/*
@funtion	void InitialCana(void)
@brief		
@param		void
@return		void
@remark	
	-	
*/
void InitialCana(void)
{
	// GPIO 포트 초기화 및 CANA, CANB 모듈용 RX, TX 포트 설정
	GPIO_setPinConfig(GPIO_70_CANA_RX);
	GPIO_setPinConfig(GPIO_71_CANA_TX);

	// CAN 컨트롤러 초기화 (CANA)
	CAN_initModule(CANA_BASE);

	// 수신 버퍼 초기화
	{
		Uint16 i;
		for(i=0; i<MSG_DATA_LENGTH; i++) rxMsgData1[i] = 0;
	}

	// CANA 모듈의 비트 전송률을 1000kHz로 설정 (200MHz SYSCLK 기준)
	CAN_setBitRate(CANA_BASE, 200000000UL, 1000000, 10); 


	// CANA 모듈의 인터럽트 활성화
	CAN_enableInterrupt(CANA_BASE, CAN_INT_IE0 | CAN_INT_ERROR | CAN_INT_STATUS);

	// 인터럽트 벡터와 인터럽트 서비스 루틴 재-연결, 인터럽트 벡터 활성화
	Interrupt_register(INT_CANA0, &CanaISR);	// INT_CANA0 인터럽트 벡터에 CanaISR( ) 함수 연결

	// CANA 모듈 인터럽트 활성화
	Interrupt_enable(INT_CANA0);

	// CANA 모듈 인터럽트 신호 라인 CANINT0 활성화
	CAN_enableGlobalInterrupt(CANA_BASE, CAN_GLOBAL_INT_CANINT0);

	// CAN 메시지 전송에 사용할 송신(TX) 메시지 오브젝트 초기화
	CAN_setupMessageObject( CANA_BASE, TX_MSG_OBJ_NO1, CANA_TX_EXT_ID_TEST1,
							CAN_MSG_FRAME_EXT, CAN_MSG_OBJ_TYPE_TX, 0,
							CAN_MSG_OBJ_NO_FLAGS, MSG_DATA_LENGTH );

    // CAN 테스트 데이터를 수신하기 위한, 수신 메시지 오브젝트 초기화 (ID: 0x18FF30AD)
    CAN_setupMessageObject( CANA_BASE, RX_MSG_OBJ_NO1, CANA_RX_EXT_ID_H1_REQUEST,
                            CAN_MSG_FRAME_EXT, CAN_MSG_OBJ_TYPE_RX, 0,
                            CAN_MSG_OBJ_RX_INT_ENABLE, MSG_DATA_LENGTH  );

	// CAN-A모듈 활성화 (Start operations)
	CAN_startModule(CANA_BASE);
}



/*
@funtion	__interrupt void CanaISR(void)
@brief		
@param		void
@return		__interrupt void
@remark	
	-	
*/
__interrupt void CanaISR(void)
{
	Uint32 status;

	// 인터럽트 발생 원인을 파악하기 위해 CAN-A 모듈의 인터럽트 상태(Interrupt Status) 확인
	status = CAN_getInterruptCause(CANA_BASE);

	switch(status)
	{
		case CAN_INT_INT0ID_STATUS :	// (메시지 수신이 아닌) CAN 컨트롤러 상태 변화에 따른 인터럽트 발생이라면, 해당 상태 확인
		{
			// 컨트롤러 상태 확인 (Read the CAN controller status)
			status = CAN_getStatus(CANA_BASE);

			// 오류(Error)가 발생했는지 확인
			if(	((status & ~(CAN_STATUS_RXOK)) != CAN_STATUS_LEC_MSK) && ((status & ~(CAN_STATUS_RXOK)) != CAN_STATUS_LEC_NONE))
			{
				// 오류가 발생했음을 알리기 위해 Flag 변수를 1로 설정
				errorFlag = 1;
			}

			break;
		}

		case RX_MSG_OBJ_NO1 :	// CANA 수신 메시지 오브젝트 2번에 의한 인터럽트일 경우, 수신된 데이터 확인
		{
			// 수신된 데이터 rxMsgData1[] 에 저장
			CAN_readMessage(CANA_BASE, RX_MSG_OBJ_NO1, (uint16_t *)rxMsgData1);

			// CAN 메시지 오브젝트 인터럽트 클리어
			CAN_clearInterruptStatus(CANA_BASE, RX_MSG_OBJ_NO1);

			// CSU_CAN 수신 완료 플래그 세트 (recvCanMessage()에서 파싱)
			bCanRxFlag = true;

			break;
		}

		default :
		{
			// 예상되지 않은 일로 인터럽트 발생 시, 이곳에서 처리
			asm("  NOP"); // No Operation
			
			break;
		}
	}

	// CAN 전역(Global) 인터럽트 Flag Clear
	CAN_clearGlobalInterruptStatus(CANA_BASE, CAN_GLOBAL_INT_CANINT0);

	// INT_CANA0 인터럽트 벡터가 포함된 CPU 인터럽트 확장그룹 9번의 Acknowledge 비트 클리어
	Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}




/*
@funtion	void SendCanaMessage(Uint16 aMsgOBJ, Uint16 aLen, Uint16 aTxData[])
@brief		
@param		[ Uint16 aMsgOBJ ]	
@param		[ Uint16 aLen ]	
@param		[ Uint16 aTxData[] ]	
@return		void
@remark	
	-	
*/
void SendCanaMessage(Uint16 aMsgOBJ, Uint16 aLen, Uint16 aTxData[])
{
	// CAN-A 모듈(TX, Polling)에서 CAN-B 모듈(RX & Verify, Interrupt)로 테스트 데이터 송신
	CAN_sendMessage(CANA_BASE, aMsgOBJ, aLen, (uint16_t *)aTxData);
}
