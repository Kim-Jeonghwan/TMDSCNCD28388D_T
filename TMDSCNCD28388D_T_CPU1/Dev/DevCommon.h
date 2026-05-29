/**********************************************************************

	Nexcom Co., Ltd.
	Copyright 2021. All Rights Reserved.

	Filename		: DevCommon.h
	Version			: 00.00
	Description		: 
	Tracebility		: 
	Programmer	    :
	Last Updated	: 2026. 01. 21.

**********************************************************************/

/*
 * Modification History
 * --------------------
 * 
 * 
*/


#ifndef DEVCOMMON_H
#define DEVCOMMON_H

/* ************************** [[   include  ]]  *********************************************************** */
#include "main.h"


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



#endif	// #ifndef DEVOMMON_H



