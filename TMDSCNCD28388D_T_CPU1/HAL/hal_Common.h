/**********************************************************************
	Nexcom Co., Ltd.
	Filename         : hal_Common.h
	Version          : 00.00
	Description      : CPU1 공통 자료형 및 데이터 구조 정의 헤더
	Programmer       : Kim Jeonghwan
	Last Updated     : 2026. 06. 23. (코딩 규칙 준수 정비)
**********************************************************************/

/*
 * Modification History
 * --------------------
 * 2026. 06. 23. - 코딩 규칙 준수 정비 (작성자, 설명 기입 및 이력 보완)
 */


#ifndef HAL_COMMON_H
#define HAL_COMMON_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main_cpu1.h"


/* ************************** [[   define   ]]  *********************************************************** */



/* ************************** [[   enum or struct   ]]  *************************************************** */
typedef union
{
    uint32_t 		u32;
	float32_t f32;

    struct
    {
	    uint16_t B0:8u;
	    uint16_t B1:8u;
	    uint16_t B2:8u;
	    uint16_t B3:8u;
    } byte;
}onConv32;


typedef union
{
    uint16_t u16;

    struct
    {
	    uint16_t B0:8u;
	    uint16_t B1:8u;
    } byte;

	struct
	{
		uint16_t b00:1u;
		uint16_t b01:1u;
		uint16_t b02:1u;
		uint16_t b03:1u;
		uint16_t b04:1u;
		uint16_t b05:1u;
		uint16_t b06:1u;
		uint16_t b07:1u;
		uint16_t b08:1u;
		uint16_t b09:1u;
		uint16_t b10:1u;
		uint16_t b11:1u;
		uint16_t b12:1u;
		uint16_t b13:1u;
		uint16_t b14:1u;
		uint16_t b15:1u;
	} bit;
}onConv16;


/* ************************** [[   global   ]]  *********************************************************** */


/* ************************** [[  function  ]]  *********************************************************** */



#endif	// #ifndef HAL_COMMON_H



