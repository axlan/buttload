#ifndef USART_H
#define USART_H

//***************************************************************************
//
//  File........: usart.c
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Compiler....: IAR EWAAVR 2.28a
//
//  Description.: AVR Butterfly USART routines
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20030116 - 1.0  - Created                                       - LHM
//
//***************************************************************************

// INCLUDES:
#include <avr/io.h>
#include <avr/interrupt.h>

#include "Main.h"
#include "RingBuff.h"
#include "Timeout.h"

// MACROS AND DEFINES:
#define USART_TX_ON           1
#define USART_TX_OFF          0
#define USART_RX_ON           1
#define USART_RX_OFF          0

#define USART_BAUDRATE        115200
#define USART_DOUBLESPEED     TRUE

#define USART_BAUDVALUE       (((F_CPU * ((USART_DOUBLESPEED == TRUE)? 2 : 1)) / (USART_BAUDRATE * 16UL)) - 1)

#define USART_ENABLE(Tx,Rx)   UCSRB = (((Rx<<RXEN) | (Tx<<TXEN)) | (UCSRB & (1 << RXCIE)));

// PROTOTYPES:
void USART_Init(unsigned int baudrate);
void USART_TxString(char *data);
void USART_Tx(char data);
char USART_Rx(void);

#endif
