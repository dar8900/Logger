#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#define ARDUINO

#define SDCARD_INSERT	

#undef DBG

#pragma pack(1)
typedef struct 
{
	uint32_t timeStamp;
	union 
	{
		uint32_t dataInt;
		float dataFloat;
	}Data;
	
}LOG_T;
#pragma pack()

#ifdef ARDUINO
#include <Arduino.h>

#ifndef SDCARD_INSERT
#include <EEPROM.h>
#endif

#ifdef DBG
#define SCRIVI(roba) (Serial.println(roba))
#else
#define SCRIVI(roba)
#endif

#ifdef SDCARD_INSERT
#include <SPI.h>
#include <SD.h>

#define CREATE_DIR 	true
#define REMOVE_DIR  false

// Generic catch-all implementation.
template <typename T_ty> struct TypeInfo { static const char * name; };
template <typename T_ty> const char * TypeInfo<T_ty>::name = "unknown";
// Handy macro to make querying stuff easier.
#define TYPE_NAME(var) TypeInfo< typeof(var) >::name
// Handy macro to make defining stuff easier.
#define MAKE_TYPE_INFO(type)  template <> const char * TypeInfo<type>::name = #type;
// Type-specific implementations.
MAKE_TYPE_INFO( int )
MAKE_TYPE_INFO( float )
MAKE_TYPE_INFO( double )
MAKE_TYPE_INFO( String  ) 
MAKE_TYPE_INFO( LOG_T  ) 

#endif // ifdef SDCARD_INSERT

#else // #ifdef ARDUINO

using namespace std;

#ifdef DBG
#define SCRIVI(roba) (cout << roba << endl)
#else
#define SCRIVI(roba)
#endif

#define EEPROM_SIZE	 80

#endif // #ifdef ARDUINO



#define MEMORY_NOT_INITIALIZED	0xFFFFFFFF

#define LOG_TYPE	 LOG_T


typedef enum
{
	SAVE_SINGLE_FAILED =   0,
	SAVE_SINGLE_OK,			
	LOAD_SINGLE_FAILED,	    
	LOAD_SINGLE_OK,		
	SAVE_TOT_FAILED,
	SAVE_TOT_OK,			
	LOAD_TOT_FAILED,	    
	LOAD_TOT_OK,	
	LOAD_SET_FAILED,
	LOAD_SET_OK,			 
	CLEAR_OK,		
	CLEAR_FAILED,	
	INIT_OK,
	OVER_MEMORY_SETTED,
	MEMORY_NOT_INIT,
	SD_OPEN = 100,
	SD_NOT_OPEN,
	SD_FORMAT_OK,
	SD_FORMAT_FAILED,
	SD_WRITE_OK,
	SD_WRITE_FAILED,
	SD_READ_OK,
	SD_READ_FAILED,
	SD_CREATE_DIR_OK,
	SD_CREATE_DIR_FAILED,
	SD_REMOVE_DIR_OK,
	SD_REMOVE_DIR_FAILED,	
	SD_REMOVE_OK,
	SD_REMOVE_FAILED,
	MAX_ERR_CODE = 255
}LOGGER_ERR_CODE;



#ifndef ARDUINO
template <typename EeType> class EEP
{
public:
	EEP(uint32_t MemorySize);
	void put(uint32_t Addr, EeType Data);
	EeType get(uint32_t Addr, EeType &Data);
	uint32_t length();
	void update(uint32_t Addr, EeType Data);
private:
	uint32_t MaxMemory;	
	EeType *Memory;
};
#endif // ifndef ARDUINO

template <typename LogType> class LOGGER
{
public:

#ifndef SDCARD_INSERT	
	uint8_t begin(uint32_t MemorySizeSetted = MEMORY_NOT_INITIALIZED);
	uint8_t setMemory(uint32_t MemorySizeSetted);
	uint8_t saveAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t saveSingleData(LogType SingleData);
	uint8_t loadAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t loadLastData(LogType *SingleData);
	uint8_t loadSetData(LogType *Buffer, uint32_t BufferSize, uint32_t SetInit, uint32_t SetEnd);
	uint8_t clearMemory();
#else
	uint8_t begin(uint8_t SdCS = 10);
	uint8_t writeInFile(LogType Data, char *FileName);
	// uint8_t readInFile(LogType *Data, char *FileName, uint32_t HowMuchData);
	uint8_t removeFile(char *FileName);
	uint8_t manageDir(char *DirName, bool CreateOrRemove = CREATE_DIR);

#endif	

	void stampDbgMsg();
	bool enableDbgMsg = false;

private:

#ifndef SDCARD_INSERT	
	uint32_t MemorySize = MEMORY_NOT_INITIALIZED;
	uint32_t LastDataAddr = 0;
	uint16_t TypeSize = 1;
	bool MemoryFull = false;
#endif	
	uint8_t RetValue;
};


#ifndef ARDUINO
template <typename EeType> EEP<EeType>::EEP(uint32_t MemorySize)
{
	MaxMemory = MemorySize;
	Memory = new EeType[MaxMemory];
}	

template <typename EeType> void EEP<EeType>::put(uint32_t Addr, EeType Data)
{
	Memory[Addr] = Data;
}
template <typename EeType> EeType EEP<EeType>::get(uint32_t Addr, EeType &Data)
{
	Data = Memory[Addr];
}
template <typename EeType> uint32_t EEP<EeType>::length()
{
	return MaxMemory;
}

template <typename EeType> void EEP<EeType>::update(uint32_t Addr, EeType Data)
{
	EeType Tmp;
	get(Addr, Tmp);
	if(memcmp(&Tmp, &Data, sizeof(EeType)) != 0)
		put(Addr, Data);
}

static EEP<LOG_TYPE> EEPROM(EEPROM_SIZE);
#endif

#ifndef SDCARD_INSERT
template <typename LogType> uint8_t LOGGER<LogType>::begin(uint32_t MemorySizeSetted)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(MemorySizeSetted == 0)
	{
		MemorySize = MEMORY_NOT_INITIALIZED;
		Ret = MEMORY_NOT_INIT;
	}
	else
	{
		TypeSize = sizeof(LogType);
		MemorySize = MemorySizeSetted * TypeSize; 
		Ret = INIT_OK;
		if(MemorySize > EEPROM.length())
		{
			MemorySize = EEPROM.length();	
			Ret = OVER_MEMORY_SETTED;
		}		
	}
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::setMemory(uint32_t MemorySizeSetted)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(MemorySizeSetted == 0)
	{
		MemorySize = MEMORY_NOT_INITIALIZED;
		Ret = MEMORY_NOT_INIT;
	}
	else
	{
		MemorySize = MemorySizeSetted * TypeSize; 
		Ret = INIT_OK;
		if(MemorySize > EEPROM.length())
		{
			MemorySize = EEPROM.length();
			Ret = OVER_MEMORY_SETTED;
		}
		Ret += clearMemory();
	}
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::saveAllData(LogType *AllBuffer, uint32_t BufferSize)
{
	int Addr = 0;
	uint8_t Ret = MAX_ERR_CODE;
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + SAVE_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}
	if(BufferSize * TypeSize > MemorySize)
	{
		Ret = SAVE_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	if(BufferSize > MemorySetted)
	{
		Ret = SAVE_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}		
	if(LastDataAddr >= MemorySize)
	{
		LastDataAddr = 0;
		MemoryFull = true;
	}
	for(Addr = 0; Addr < BufferSize; Addr++)
	{
		EEPROM.put((Addr * TypeSize), AllBuffer[Addr]);
	}
	LastDataAddr += (Addr * TypeSize);
	Ret = SAVE_TOT_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;	
}

template <typename LogType> uint8_t LOGGER<LogType>::saveSingleData(LogType SingleData)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + SAVE_SINGLE_FAILED;
		RetValue = Ret;	
		if(enableDbgMsg)
			stampDbgMsg();			
		return Ret;	
	}
	if(LastDataAddr >= MemorySize)
	{
		LastDataAddr = 0;
		MemoryFull = true;
	}
	EEPROM.put(LastDataAddr, SingleData);
	LastDataAddr += TypeSize;
	Ret = SAVE_SINGLE_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;		
}

template <typename LogType> uint8_t LOGGER<LogType>::loadAllData(LogType *AllBuffer, uint32_t BufferSize)
{
	int Addr = 0;
	uint8_t Ret = MAX_ERR_CODE;
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + LOAD_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}
	if(BufferSize * TypeSize > MemorySize)
	{
		Ret = LOAD_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	if(LastDataAddr == 0)
	{
		Ret = LOAD_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	if(BufferSize > MemorySetted)
	{
		Ret = LOAD_TOT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	for(Addr = 0; Addr < BufferSize; Addr++)
	{
		EEPROM.get((Addr * TypeSize), AllBuffer[Addr]);
	}
	if(MemoryFull)
	{
		LogType Tmp1, Tmp2;
		int LastIndex = LastDataAddr / TypeSize;
		for(int i = 0; i < LastIndex; i++)
		{
			Tmp1 = AllBuffer[BufferSize - 1 - i];
			Tmp2 = AllBuffer[LastIndex - 1 - i];
			for(int j = LastIndex - 1 - i; j < BufferSize - 1; j++)
			{
				AllBuffer[j] = AllBuffer[j + 1];
			}
			AllBuffer[BufferSize - 1 - i] = Tmp2;
		}
	}
	Ret = LOAD_TOT_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::loadLastData(LogType *SingleData)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + LOAD_SINGLE_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}
	if(LastDataAddr >= TypeSize)
		EEPROM.get(LastDataAddr - TypeSize, *SingleData);
	Ret = LOAD_SINGLE_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;		
}

template <typename LogType> uint8_t LOGGER<LogType>::loadSetData(LogType *Buffer, uint32_t BufferSize, uint32_t SetInit, uint32_t SetEnd)
{
	uint8_t Ret = MAX_ERR_CODE;
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + LOAD_SET_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}
	if(BufferSize * TypeSize > MemorySize)
	{
		Ret = LOAD_SET_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
    if(SetInit > MemorySetted || SetEnd > MemorySetted)
	{
		Ret = LOAD_SET_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	if((SetEnd - SetInit) > BufferSize)
	{
		Ret = LOAD_SET_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}	
	for(int Addr = 0; Addr < (SetEnd - SetInit); Addr++)
	{
		EEPROM.get((SetInit * TypeSize) + (Addr * TypeSize), Buffer[Addr]);
	}
	Ret = LOAD_SET_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::clearMemory()
{
	uint8_t Ret = MAX_ERR_CODE;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
	{
		Ret = MEMORY_NOT_INIT + CLEAR_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;
	}
	LogType DataVar;
	memset(&DataVar, 0, TypeSize);
	for(int i = 0; i < MemorySize; i++)
		EEPROM.update(i, DataVar);
	MemoryFull = false;
	Ret = CLEAR_OK;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;	
}

#else // ifndef SDCARD_INSERT

template <typename LogType> uint8_t LOGGER<LogType>::begin(uint8_t SdCS)
{
	Sd2Card card;
	SdVolume volume;
	uint8_t Ret = MAX_ERR_CODE;
	if(!SD.begin(SdCS))
	{
		Ret = SD_NOT_OPEN;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;	
	}
	if(!volume.init(card))
	{
		Ret = SD_FORMAT_FAILED;
		RetValue = Ret;
		if(enableDbgMsg)
			stampDbgMsg();		
		return Ret;	
	}
	Ret = SD_OPEN;
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();	
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::writeInFile(LogType Data, char *FileName)
{
	uint8_t Ret = MAX_ERR_CODE;
	File File2Write = SD.open(FileName, FILE_WRITE);
	String DataFileType = String(TYPE_NAME(Data));
	if(File2Write)
	{
		if(DataFileType != "unknown")
		{
			if(DataFileType != "LOG_T")
			{
				File2Write.println(Data);
			}
			else
			{
				LOG_T *DataCpy = &Data;
				time_t curtime = (time_t)DataCpy->timeStamp;
				File2Write.print(ctime(&curtime));
				File2Write.print("->");
				File2Write.print(DataCpy->Data.dataFloat);
				File2Write.println();
				File2Write.close();
			}
			Ret = SD_WRITE_OK;
		}
		else
			Ret = SD_WRITE_FAILED;
	}
	else
		Ret = SD_WRITE_FAILED;

	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();
	return Ret;
}

// template <typename LogType> uint8_t LOGGER<LogType>::readInFile(LogType *Data, char *FileName, uint32_t HowMuchData)
// {
// 	uint8_t Ret = MAX_ERR_CODE;
// 	// File File2Read = SD.open(FileName, FILE_READ);
// 	// String DataFileType = String(TYPE_NAME(*Data));
// 	// if(File2Read)
// 	// {
// 	// 	if(File2Read.available() > 0)
// 	// 	{
// 	// 		if(DataFileType != "unknown")
// 	// 		{
// 	// 			if(DataFileType != "LOG_T")
// 	// 			{
// 	// 				for(int i = 0; i < HowMuchData;)
// 	// 				{
// 	// 					if(DataFileType == "int" || DataFileType == "float")
// 	// 					{
// 	// 						for(int i = 0; i < 4; i++)
// 	// 						{
// 	// 							*Data |= File2Read.read();
// 	// 							if(i < 3)
// 	// 								*Data <<= 8;
// 	// 						}
// 	// 					}
// 	// 				}
// 	// 			}	
// 	// 			else
// 	// 			{

// 	// 			}		
// 	// 		}
// 	// 		else
// 	// 			Ret = SD_READ_FAILED;
// 	// 	}
// 	// 	else
// 	// 		Ret = SD_READ_FAILED;
// 	// }
// 	// else
// 	// 	Ret = SD_READ_FAILED;
// 	// RetValue = Ret;
// 	// if(enableDbgMsg)
// 	// 	stampDbgMsg();
// 	return Ret;	
// }


template <typename LogType> uint8_t LOGGER<LogType>::removeFile(char *FileName)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(SD.exists(FileName))
	{
		SD.remove(FileName);
		Ret = SD_REMOVE_OK;
	}
	else
		Ret = SD_REMOVE_FAILED;

	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();
	return Ret;
}

template <typename LogType> uint8_t LOGGER<LogType>::manageDir(char *DirName, bool CreateOrRemove)
{
	uint8_t Ret = MAX_ERR_CODE;
	if(CreateOrRemove == CREATE_DIR)
	{
		if(SD.exists(DirName))
			Ret = SD_CREATE_DIR_FAILED;
		else
		{
			SD.mkdir(DirName);
			Ret = SD_CREATE_DIR_OK;
		}
	}
	else
	{
		if(!SD.exists(DirName))
			Ret = SD_REMOVE_DIR_FAILED;
		else
		{
			SD.rmdir(DirName);
			Ret = SD_REMOVE_DIR_OK;
		}
	}
	RetValue = Ret;
	if(enableDbgMsg)
		stampDbgMsg();
	return Ret;	
}


#endif // ifndef SDCARD_INSERT

template <typename LogType> void LOGGER<LogType>::stampDbgMsg()
{
    switch(RetValue)
    {
        case SAVE_SINGLE_FAILED:   
            SCRIVI("SCRITTURA SINGOLA FALLITA");
            break;
        case LOAD_SINGLE_FAILED:
            SCRIVI("LETTURA SINGOLA FALLITA");
            break;
        case SAVE_TOT_FAILED:   
            SCRIVI("SCRITTURA TOTALE FALLITA");
            break;
        case LOAD_TOT_FAILED:
            SCRIVI("LETTURA TOTALE FALLITA");
            break;            
        case CLEAR_FAILED:
            SCRIVI("CANCELLAZIONE FALLITA");
            break;
        case OVER_MEMORY_SETTED:
            SCRIVI("MEMORIA SOVRADIMENSIONATA, IMPOSTATA QUELLA DI DEFAULT");
            break;
        case (OVER_MEMORY_SETTED + CLEAR_OK):
            SCRIVI("MEMORIA SOVRADIMENSIONATA, IMPOSTATA QUELLA DI DEFAULT, RESET MEMORIA OK");
            break;
        case (MEMORY_NOT_INIT + SAVE_SINGLE_FAILED):
            SCRIVI("MEMORIA NON INIZIALIZZATA, SCRITTURA SINGOLA FALLITA");
            break;
        case (MEMORY_NOT_INIT + LOAD_SINGLE_FAILED):
            SCRIVI("MEMORIA NON INIZIALIZZATA, LETTURA SINGOLA FALLITA");
            break;            
        case (MEMORY_NOT_INIT + SAVE_TOT_FAILED):
            SCRIVI("MEMORIA NON INIZIALIZZATA, SCRITTURA TOTALE FALLITA");
            break;
        case (MEMORY_NOT_INIT + LOAD_TOT_FAILED):
            SCRIVI("MEMORIA NON INIZIALIZZATA, LETTURA TOTALE FALLITA");
            break;
        case (MEMORY_NOT_INIT + CLEAR_FAILED):
            SCRIVI("MEMORIA NON INIZIALIZZATA, CANCELLAZIONE FALLITA");
            break;   
		case SD_OPEN:
			SCRIVI("SD APERTA CON SUCCESSO");
			break;
		case SD_NOT_OPEN:
			SCRIVI("APERTURA SD FALLITA");
			break;
		case SD_FORMAT_OK:
			SCRIVI("FORMATO FILE SYSTEM SD CORRETTO");
			break;
		case SD_FORMAT_FAILED:
			SCRIVI("FORMATO FILE SYSTEM SD ERRATO (FAT 16 O FAT 32)");
			break;
		case SD_WRITE_OK:
			SCRIVI("SCRITTURA SD RIUSCITA");
			break;
		case SD_WRITE_FAILED:
			SCRIVI("SCRITTURA SD FALLITA");
			break;
		case SD_READ_OK:
			SCRIVI("LETTURA SD RIUSCITA");
			break;
		case SD_READ_FAILED:
			SCRIVI("LETTURA SD FALLITA");
			break;
		case SD_CREATE_DIR_OK:
			SCRIVI("CREAZIONE DIRECTORY SD RIUSCITA");
			break;
		case SD_CREATE_DIR_FAILED:
			SCRIVI("CREAZIONE DIRECTORY SD FALLITA");
			break;
		case SD_REMOVE_DIR_OK:
			SCRIVI("RIMOZIONE DIRECTORY SD RIUSCITA");
			break;
		case SD_REMOVE_DIR_FAILED:
			SCRIVI("RIMOZIONE DIRECTORY SD FALLITA");			
			break;
		case SD_REMOVE_OK:
			SCRIVI("FILE ELIMINATO CORRETTAMENTE");		
			break;
		case SD_REMOVE_FAILED:
			SCRIVI("ELIMINAZIONE FILE NON RIUSCITA");		
			break;
		case MAX_ERR_CODE:
			SCRIVI("ERRORE SCONOSCIUTO");
			break;			                     
        default:
            SCRIVI("TUTTO OK");
            break;
    }
	RetValue = MAX_ERR_CODE;
}



#endif