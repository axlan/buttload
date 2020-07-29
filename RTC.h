//*****************************************************************************
//
//  File........: RTC.h
//
//  Author(s)...: ATMEL Norway
//
//  Target(s)...: ATmega169
//
//  Description.: Functions for RTC.c
//
//  Revisions...: 1.0
//
//  YYYYMMDD - VER. - COMMENT                                       - SIGN.
//
//  20021015 - 1.0  - File created                                  - LHM
//
//*****************************************************************************



//  Function declarations
void ShowUptime(uint32_t totalSeconds);
void ShowTimeMinutes(uint32_t totalSeconds, char seperator);
