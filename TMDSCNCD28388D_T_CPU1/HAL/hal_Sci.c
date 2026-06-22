/**********************************************************************
    Nexcom Co., Ltd.
    Filename         : hal_Sci.c
    Version          : 00.00
    Description      : CPU1 SCI(UART) 통신 드라이버 소스
    Programmer       : Kim Jeonghwan
    Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비 및 매크로 이동)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자 기입, 매크로 상수 이동 및 이력 보완)
 */


/* ************************** [[   include  ]]  *********************************************************** */
#include "hal_Sci.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   global   ]]  *********************************************************** */
static stQsci	xQueSCI_PC;


/* ************************** [[  static prototype  ]]  *************************************************** */
static void initScia_SCI_PC(void);
static void setupSciaPC_GPIO(void);
static void setupSciaPC_Interrupt(void);
static void setupSciaPC_HW(void);


static void enqueueSci(stQsci *pstQ, uint16_t Data);

static uint16_t dequeueSci(stQsci *pstQ, uint16_t *pData);


/* ************************** [[  function  ]]  *********************************************************** */
/*
@funtion    void Initial_SCI(void)
@brief      SCI PC 통신 드라이버 초기화
@param      void
@return     void
@remark 
    - 큐 초기화 및 하드웨어, GPIO 설정을 일괄 수행합니다.
*/
void Initial_SCI(void)
{
	initScia_SCI_PC();

	memset(&xQueSCI_PC, 0u, sizeof(xQueSCI_PC));
}


/*
@funtion    static void initScia_SCI_PC(void)
@brief      SCIA 포트 디바이스 내부 초기화 호출
@param      void
@return     static void
*/
static void initScia_SCI_PC(void)
{
	//
	// Initialize the Device Peripherals:
	//
	setupSciaPC_GPIO();
	setupSciaPC_Interrupt();
	setupSciaPC_HW();
}


/*
@funtion    static void setupSciaPC_GPIO(void)
@brief      SCIA PC 통신용 GPIO 핀 설정 (RX: Pullup, TX: Standard)
@param      void
@return     static void
*/
static void setupSciaPC_GPIO(void)
{
	//
	// GPIO28 핀을 SCI Rx로 설정
	//
	GPIO_setControllerCore(SCI_PC_GPIO_PIN_SCIA_RXD, GPIO_CORE_CPU1);
	GPIO_setPinConfig(SCI_PC_GPIO_CFG_SCIA_RXD);
	GPIO_setDirectionMode(SCI_PC_GPIO_PIN_SCIA_RXD, GPIO_DIR_MODE_IN);
	GPIO_setPadConfig(SCI_PC_GPIO_PIN_SCIA_RXD, GPIO_PIN_TYPE_PULLUP);	// 2026. 05. 15. RX 는 대부분 pull-up 저항이 필요함.
	GPIO_setQualificationMode(SCI_PC_GPIO_PIN_SCIA_RXD, GPIO_QUAL_ASYNC);

	//
	// GPIO29 핀을 SCI Tx로 설정
	//
	GPIO_setControllerCore(SCI_PC_GPIO_PIN_SCIA_TXD, GPIO_CORE_CPU1);
	GPIO_setPinConfig(SCI_PC_GPIO_CFG_SCIA_TXD);
	GPIO_setDirectionMode(SCI_PC_GPIO_PIN_SCIA_TXD, GPIO_DIR_MODE_OUT);
	GPIO_setPadConfig(SCI_PC_GPIO_PIN_SCIA_TXD, GPIO_PIN_TYPE_STD);
	GPIO_setQualificationMode(SCI_PC_GPIO_PIN_SCIA_TXD, GPIO_QUAL_ASYNC);
}


/*
@funtion    static void setupSciaPC_Interrupt(void)
@brief      SCIA 인터럽트 ISR 등록 및 활성화
@param      void
@return     static void
*/
static void setupSciaPC_Interrupt(void)
{
	//
	// 이 예제에서 사용하는 인터럽트들을 본 파일 내의 ISR 함수에 매핑
	//
	Interrupt_register(INT_SCIA_RX, isrScia_SCI_PC);

	Interrupt_enable(INT_SCIA_RX);
	
	Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);
}


/*
@funtion    static void setupSciaPC_HW(void)
@brief      SCIA 하드웨어 파라미터 및 FIFO, 인터럽트 플래그 설정
@param      void
@return     static void
*/
static void setupSciaPC_HW(void)
{
    //
    // 8 데이터 비트, 1 정지 비트, 패리티 없음. Baud Rate(보레이트) 115200.
    //
    SCI_setConfig(SCIA_BASE, DEVICE_LSPCLK_FREQ, 115200u, (SCI_CONFIG_WLEN_8 |
                                                         SCI_CONFIG_STOP_ONE |
                                                         SCI_CONFIG_PAR_NONE));
    SCI_enableModule(SCIA_BASE);
    SCI_resetChannels(SCIA_BASE);
    SCI_enableFIFO(SCIA_BASE);

    //
    // RX 및 TX FIFO 인터럽트 활성화
    //
    SCI_enableInterrupt(SCIA_BASE, SCI_INT_RXFF);
    SCI_disableInterrupt(SCIA_BASE, SCI_INT_RXERR);

    //
    // 송신 FIFO는 FIFO 상태가 16 단어 중 2개 이하일 때 인터럽트 발생
    // 수신 FIFO는 FIFO 상태가 16 단어 중 2개 이상일 때 인터럽트 발생
    //
    SCI_setFIFOInterruptLevel(SCIA_BASE, SCI_FIFO_TX1, SCI_FIFO_RX1);
    SCI_performSoftwareReset(SCIA_BASE);

    SCI_resetTxFIFO(SCIA_BASE);
    SCI_resetRxFIFO(SCIA_BASE);
}


/*
@funtion    __interrupt void isrScia_SCI_PC(void)
@brief      SCIA 수신 인터럽트 서비스 루틴
@param      void
@return     __interrupt void
@remark 
    - FIFO에서 1바이트씩 데이터를 읽어 패킷 조립(Frame 상태머신)을 수행합니다.
*/
__interrupt void isrScia_SCI_PC(void)
{
	static stSciA	xRcvSCI_PC;

    uint16_t Data[1u];

	// FIFO에 데이터가 있을 때만 루프를 돌며 읽는 것이 안전하지만, 
    // 현재 레벨이 1이므로 1바이트씩 처리하는 로직을 유지
    SCI_readCharArray(SCIA_BASE, Data, 1u);

  	switch(xRcvSCI_PC.Frame)
  	{
  	case eSciA_SOF:
  		if(Data[0u] == 0x7Eu)
  		{
  			xRcvSCI_PC.Frame	= eSciA_MSGID;
  			xRcvSCI_PC.POS	= 0u;
  			xRcvSCI_PC.CRC	= 0u;
  		}
  	break;

  	case eSciA_MSGID:
		xRcvSCI_PC.MSGID	= Data[0u];
		xRcvSCI_PC.Frame	= eSciA_LEN;
  	break;

  	case eSciA_LEN:
		xRcvSCI_PC.LEN	= Data[0u];
		xRcvSCI_PC.CRC	= Data[0u];			// LEN 포함 합산
		xRcvSCI_PC.POS = 0u;				// POS 초기화 보장
        
        if(xRcvSCI_PC.LEN > 0u)
        {
            xRcvSCI_PC.LEN--;              // 보정: LEN 필드가 자기 자신을 포함하므로 실제 데이터는 LEN-1개
            xRcvSCI_PC.Frame = eSciA_DATA;
        }
        else
        {
            xRcvSCI_PC.Frame = eSciA_CRC;   // 데이터가 없는 경우
        }
  	break;

  	case eSciA_DATA:
		xRcvSCI_PC.DATA[xRcvSCI_PC.POS++] = (Data[0u] & 0x00FFu); // 하위 8비트만 명시
		xRcvSCI_PC.CRC += (Data[0u] & 0x00FFu);
		xRcvSCI_PC.LEN--;                          // 남은 개수 하나 감소
        if(xRcvSCI_PC.LEN == 0u)                   // 다 받았으면 CRC 단계로
        {
            xRcvSCI_PC.Frame = eSciA_CRC;
        }	
  	break;

  	case eSciA_CRC:
  		if((xRcvSCI_PC.CRC & 0xFFu) == Data[0u])
  		{
			xRcvSCI_PC.Frame	= eSciA_EOT;	// 맞으면 EOT 대기
		}
		else
		{
			xRcvSCI_PC.Frame	= eSciA_SOF;	// 틀리면 SOF 로
		}
  	break;  	

  	case eSciA_EOT:
		if(Data[0u] == 0x0Du)	// 마지막 바이트 0x0D 인지 체크
		{
			recvSciPcMessage(xRcvSCI_PC.MSGID, xRcvSCI_PC.DATA);
		}
		xRcvSCI_PC.Frame	= eSciA_SOF;	// 성공하든 실패하든 초기화
  	break;  	
  	
  	default:
  		xRcvSCI_PC.Frame = eSciA_SOF;
  	break;
  	}

    SCI_clearOverflowStatus(SCIA_BASE);

    SCI_clearInterruptStatus(SCIA_BASE, SCI_INT_RXFF);

    //
    // PIE ack 승인 발행
    //
    Interrupt_clearACKGroup(INTERRUPT_ACK_GROUP9);

}


/*
@funtion    void xmtScia_SCI_PC(uint16_t data[], uint16_t len)
@brief      SCI 송신 큐에 데이터 삽입
@param      data[]: 송신할 데이터 배열
@param      len: 전송 길이
@return     void
@remark 
    - 즉시 전송하지 않고 송신 큐(enqueueSci)에 적재하여 백그라운드 태스크에서 비동기로 전송합니다.
*/
void xmtScia_SCI_PC(uint16_t data[], uint16_t len)
{
#if 1 // 2025-08-05 9:13:57
	uint16_t i = 0u;

	for(i = 0u; i < len; i++)
	{
		enqueueSci(&xQueSCI_PC, data[i]);
	}
#else
	SCI_writeCharArray(SCIA_BASE, data, len);
#endif // #if 0 // 2025-08-05 9:13:57
}



/*
@funtion    void sendScia_SCI_PC(void)
@brief      비동기 SCI 데이터 송신 처리 (백그라운드 루프 폴링)
@param      void
@return     void
@remark 
    - 큐에서 데이터를 하나씩 꺼내어 SCIA TX 버퍼의 여유가 있을 때마다 물리적 전송을 수행합니다.
*/
void sendScia_SCI_PC(void)
{
    uint16_t i = 0u;
    uint16_t len = 0u;
    uint16_t popData = 0u;
    uint16_t sendData[20u] = {0u};		// 10 에서 20으로 변경

    for(i = 0u; i < 20u; i++)
    {
        if(dequeueSci(&xQueSCI_PC, &popData) == 1u)
        {
            sendData[len ++] = popData;
        }
    }

    if(len > 0u)
    {
        SCI_writeCharArray(SCIA_BASE, sendData, len);
    }
}




/*
@funtion    static void enqueueSci(stQsci *pstQ, uint16_t Data)
@brief      SCI 송신 큐(원형 버퍼)에 1바이트 데이터 삽입
@param      pstQ: 대상 큐 구조체 포인터
@param      Data: 삽입할 1바이트 데이터
@return     static void
@remark 
    - 큐가 가득 차지 않았을 경우에만 데이터를 삽입하고 Rear 포인터를 증가시킵니다.
*/
static void enqueueSci(stQsci *pstQ, uint16_t Data)
{
    uint16_t nRear = 0u;

    if(pstQ != NULL)
    {
        if(pstQ->rear < QUEUE_MAX_SCI)
        {
            nRear = ((pstQ->rear + 1u) % QUEUE_MAX_SCI);

            if(nRear != pstQ->front)
            {
                pstQ->Data[pstQ->rear] = Data;
                pstQ->rear = nRear;
            }
        }
    }
}




/*
@funtion    static uint16_t dequeueSci(stQsci *pstQ, uint16_t *pData)
@brief      SCI 송신 큐(원형 버퍼)에서 1바이트 데이터 추출
@param      pstQ: 대상 큐 구조체 포인터
@param      pData: 추출된 데이터를 담을 포인터
@return     static uint16_t (1: 성공, 0: 실패)
@remark 
    - 큐가 비어있지 않을 경우 데이터를 반환하고 Front 포인터를 증가시킵니다.
*/
static uint16_t dequeueSci(stQsci *pstQ, uint16_t *pData)
{
    uint16_t result = 0u;

    if((pstQ != NULL) && (pData != NULL))
    {
        if(pstQ->front != pstQ->rear)
        {
            *pData = pstQ->Data[pstQ->front];

            if(pstQ->front <= QUEUE_MAX_SCI)
            {
                pstQ->front = (pstQ->front + 1u) % QUEUE_MAX_SCI;
            }

            result = 1u;
        }
    }

    return result;
}
