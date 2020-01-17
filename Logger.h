#ifndef LOGGER_H
#define LOGGER_H

#include <stdint.h>
#include <string.h>
#include <time.h>

#define ARDUINO

#ifdef ARDUINO
#include <Arduino.h>
#include <EEPROM.h>
#define SCRIVI(roba) (Serial.println(roba))
#else

using namespace std;
#define SCRIVI(roba) (cout << roba << endl)
#define EEPROM_SIZE	 80

#endif



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
	MAX_ERR_CODE = 255
}LOGGER_ERR_CODE;


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
#endif

template <typename LogType> class LOGGER
{
public:
	uint8_t begin(uint32_t MemorySizeSetted = MEMORY_NOT_INITIALIZED);
	uint8_t setMemory(uint32_t MemorySizeSetted);
	uint8_t saveAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t saveSingleData(LogType SingleData);
	uint8_t loadAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t loadLastData(LogType *SingleData);
	uint8_t loadSetData(LogType *Buffer, uint32_t BufferSize, uint32_t SetInit, uint32_t SetEnd);
	uint8_t clearMemory();
	void stampDbgMsg();
	bool enableDbgMsg = false;

private:
	uint32_t MemorySize = MEMORY_NOT_INITIALIZED;
	uint32_t LastDataAddr = 0;
	uint16_t TypeSize = 1;
	bool MemoryFull = false;
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