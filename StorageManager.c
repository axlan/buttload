/*
             BUTTLOAD - Butterfly ISP Programmer
				
              Copyright (C) Dean Camera, 2006.
                  dean_camera@hotmail.com
*/

#define  INC_FROM_SM
#include "StorageManager.h"

// GLOBAL VARIABLES:
uint8_t  MemoryType          = TYPE_FLASH;
uint8_t  CurrentMode         = SM_NO_SETUP;
uint16_t GPageLength         = 0;
uint8_t  WriteCmdStored      = 0;

const char StorageText[] PROGMEM = "*STORAGE MODE*";

// ======================================================================================

uint32_t SM_GetStoredDataSize(const uint8_t Type)
{
	uint32_t ProgDataSize = 0;
	uint16_t BlockStart;
	uint16_t BlockEnd;

	if (Type == TYPE_FLASH)
	{
		BlockStart = 0;
		BlockEnd   = 124;
	}
	else
	{
		BlockStart = 125;
		BlockEnd   = 249;
	}

	for (uint16_t EEPageBlock = BlockStart; EEPageBlock < BlockEnd; EEPageBlock++)
	{
		uint8_t Mask      = 0b10000000;
		uint8_t BlockData = eeprom_read_byte(&EEPROMVars.PageEraseReqFlags[EEPageBlock]);

		while (Mask)
		{
			if (BlockData & Mask)
			  ProgDataSize += DF_INTERNALDF_BUFFBYTES;

			Mask >>= 1;
		}
	}
	
	return ProgDataSize;
}

void SM_InterpretAVRISPPacket(void)
{
	uint8_t* EEPROMAddress;

	switch (PacketBytes[0])
	{
		case AICB_CMD_ENTER_PROGMODE_ISP:
			MessageSize = 2;
						
			for (uint8_t PacketB = 0; PacketB < 12; PacketB++)          // Save the enter programming mode command bytes
			  eeprom_write_byte(&EEPROMVars.EnterProgMode[PacketB], PacketBytes[PacketB]);
			
			InProgrammingMode = TRUE;                                   // Set the flag, prevent the user from exiting the V2P state machine			
			CurrentMode = SM_NO_SETUP;                                  // Clear the current mode variable
			WriteCmdStored = 0;                                         // lear Flash/EEPROM write command stored flag
			CurrAddress = 0;

			VAMM_EnterStorageMode();

			MAIN_SETSTATUSLED(MAIN_STATLED_RED);
			PacketBytes[1] = AICB_STATUS_CMD_OK;

			break;			
		case AICB_CMD_LEAVE_PROGMODE_ISP:
			MessageSize = 2;

			SM_CheckEndOfFuseLockData();                                // Check for remaining bytes to be stored and general cleanup

			TG_PlayToneSeq((InProgrammingMode && !(EraseFlagsTransReq))? TONEGEN_SEQ_PROGDONE : TONEGEN_SEQ_WAITWRITE);
		
			InProgrammingMode = FALSE;                                  // Clear the flag, allow the user to exit the V2P state machine

			DF_ENABLEDATAFLASH(FALSE);

			MAIN_SETSTATUSLED(MAIN_STATLED_GREEN);
			PacketBytes[1] = AICB_STATUS_CMD_OK;

			break;
		case AICB_CMD_READ_SIGNATURE_ISP:
			MessageSize = 4;

			PacketBytes[1] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0x01;                                      // Signature bytes all return "01" in storage mode
			PacketBytes[3] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_CHIP_ERASE_ISP:
			MessageSize = 2;

			VAMM_EraseAVRMemory();
			PM_SetProgramDataType(PM_OPT_CLEARFLAGS);
			
			PacketBytes[1] = AICB_STATUS_CMD_OK;
			
			break;
		case AICB_CMD_READ_OSCCAL_ISP:
			MessageSize = 4;

			PacketBytes[1] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs
			PacketBytes[2] = 0x00;                                      // Return 0x00 for the OSCCAL byte
			PacketBytes[3] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_PROGRAM_FUSE_ISP:
		case AICB_CMD_PROGRAM_LOCK_ISP:
			MessageSize = 3;

			if (CurrentMode != SM_LOCKFUSEBITS_WRITE)                   // First lock or fuse byte being written, set the EEPROM pointer
			{
				CurrentMode   = SM_LOCKFUSEBITS_WRITE;
				DataflashInfo.CurrBuffByte  = 0;                        // CurrBuffByte is used to store the total fuse/lock bytes written
			}

			if (PacketBytes[0] == AICB_CMD_PROGRAM_FUSE_ISP)
			{
				EEPROMAddress = &EEPROMVars.FuseBytes[DataflashInfo.CurrBuffByte][0];
				MemoryType    = TYPE_FUSE;
				PM_SetProgramDataType(PM_OPT_FUSE);
			}
			else
			{
				EEPROMAddress = &EEPROMVars.LockBytes[DataflashInfo.CurrBuffByte][0];
				MemoryType    = TYPE_LOCK;
				PM_SetProgramDataType(PM_OPT_LOCK);
			}				
			
			if (DataflashInfo.CurrBuffByte < SM_MAX_FUSELOCKBITS)
			{
				for (uint8_t FLByte = 1; FLByte < 5; FLByte++)
				{
					eeprom_write_byte(EEPROMAddress, PacketBytes[FLByte]);
					EEPROMAddress++;
				}

				DataflashInfo.CurrBuffByte++;                           // Increment the total fuse/lock bytes written counter
			}
			
			PacketBytes[1] = AICB_STATUS_CMD_OK;                        // Two CMD_OKs are always returned
			PacketBytes[2] = AICB_STATUS_CMD_OK;                        // Two CMD_OKs are always returned

			break;
		case AICB_CMD_READ_FUSE_ISP:
		case AICB_CMD_READ_LOCK_ISP:
			MessageSize = 4;
	
			if (CurrentMode != SM_LOCKFUSEBITS_READ)                    // First lock or fuse byte being read, set the EEPROM pointer
			{
				SM_CheckEndOfFuseLockData();                            // Check for remaining bytes to be stored and general cleanup
				
				DataflashInfo.CurrBuffByte = 0;
				CurrentMode  = SM_LOCKFUSEBITS_READ;
			}
			
			if (DataflashInfo.CurrBuffByte > eeprom_read_byte((PacketBytes[0] == AICB_CMD_READ_FUSE_ISP)? &EEPROMVars.TotalFuseBytes : &EEPROMVars.TotalLockBytes))  // Trying to read more fuse/lock bytes than are stored in memory
			{
				PacketBytes[2] = 0xFF;                                  // Return 0xFF for the fuse/lock byte
			}
			else
			{
				uint8_t FuseLockNum  = DataflashInfo.CurrBuffByte;
				uint8_t FuseLockByte = (PacketBytes[1] - 1);
				
				PacketBytes[2] = eeprom_read_byte((uint8_t*)(((PacketBytes[0] == AICB_CMD_READ_FUSE_ISP)? &EEPROMVars.FuseBytes[FuseLockNum][FuseLockByte] : &EEPROMVars.LockBytes[FuseLockNum][FuseLockByte])));
			}

			DataflashInfo.CurrBuffByte++;

			PacketBytes[1] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs
			PacketBytes[3] = AICB_STATUS_CMD_OK;                        // Data byte is encased in CMD_OKs

			break;
		case AICB_CMD_PROGRAM_FLASH_ISP:
		case AICB_CMD_PROGRAM_EEPROM_ISP:
			MessageSize = 2;

			if (CurrentMode != SM_DATAFLASH_WRITE)                      // First programming packet
			{
				if (PacketBytes[0] == AICB_CMD_PROGRAM_FLASH_ISP)       // Flash programming mode
				{
					EEPROMAddress = (uint8_t*)&EEPROMVars.WriteProgram; // Set the eeprom address to the Program command bytes location
					PM_SetProgramDataType(PM_OPT_FLASH);
					MemoryType  = TYPE_FLASH;
				}
				else                                                    // EEPROM programming mode
				{
					EEPROMAddress = (uint8_t*)&EEPROMVars.WriteEEPROM;  // Set the eeprom address to the EEPROM command bytes location
					PM_SetProgramDataType(PM_OPT_EEPROM);
					MemoryType  = TYPE_EEPROM;
				}

				GPageLength = 0;				
				CurrentMode = SM_DATAFLASH_WRITE;
				
				if (!(WriteCmdStored))
				{
					for (uint8_t B = 0; B < 10; B++)                        // Save the command bytes
					{
						eeprom_write_byte(EEPROMAddress, PacketBytes[B]);
						EEPROMAddress++;
					}
					
					WriteCmdStored = TRUE;
				}
			}

			uint16_t BytesToWrite = ((uint16_t)PacketBytes[1] << 8)
			                      | PacketBytes[2];

			for (uint16_t CurrByte = 0; CurrByte < BytesToWrite; CurrByte++)
			{
				VAMM_StoreByte(PacketBytes[10 + CurrByte]);
				GPageLength++;
			}

			if (!(GPageLength & SM_PAGELENGTH_FOUNDBIT) && (PacketBytes[3] & ISPCC_PROG_MODE_PAGEDONE) && (GPageLength))
			{
				eeprom_write_word(((MemoryType == TYPE_FLASH)? &EEPROMVars.PageLength : &EEPROMVars.EPageLength), GPageLength);
		
				GPageLength |= SM_PAGELENGTH_FOUNDBIT;                  // Bit 15 is used to indicate if the length has been found
			}

			PacketBytes[1] = AICB_STATUS_CMD_OK;
		
			break;
		case AICB_CMD_READ_FLASH_ISP:
		case AICB_CMD_READ_EEPROM_ISP:	
			if (CurrentMode != SM_DATAFLASH_READ)
			{
				SM_CheckEndOfFuseLockData();                            // Check for remaining bytes to be stored and general cleanup				
				CurrentMode = SM_DATAFLASH_READ;
			}

			uint16_t BytesToRead = ((uint16_t)PacketBytes[1] << 8)      // \. Load in the number of bytes that is to
			                     | PacketBytes[2];                      // /  be read into a temp variable (MSB first)
								 
			for (uint16_t ReadByte = 0; ReadByte < BytesToRead; ReadByte++)
			  PacketBytes[2 + ReadByte] = VAMM_ReadByte(); // Read in the next dataflash byte if present

			MessageSize = BytesToRead + 3;

			PacketBytes[1]               = AICB_STATUS_CMD_OK;          // Return data should be encompassed in STATUS_CMD_OKs
			PacketBytes[2 + BytesToRead] = AICB_STATUS_CMD_OK;          // Return data should be encompassed in STATUS_CMD_OKs
		
			break;
		default:
			MessageSize = 2;
			
			PacketBytes[1] = AICB_STATUS_CMD_UNKNOWN;
	}

	V2P_SendPacket();                                                   // Send the response packet

	if (!(InProgrammingMode) && (EraseFlagsTransReq))                   // Programming mode exited, cleanup
	{
		VAMM_ExitStorageMode();
		TG_PlayToneSeq(TONEGEN_SEQ_PROGDONE);
	}
}

static void SM_CheckEndOfFuseLockData(void)
{
	if (CurrentMode == SM_DATAFLASH_WRITE)
	{
		VAMM_Cleanup();
	}
	else if (CurrentMode == SM_LOCKFUSEBITS_WRITE)
	{
		// CurrBuffByte stores the total number of fuse/lock bytes written in this case:
		eeprom_write_byte(((MemoryType == TYPE_FUSE)? &EEPROMVars.TotalFuseBytes : &EEPROMVars.TotalLockBytes), DataflashInfo.CurrBuffByte);
	}
}
