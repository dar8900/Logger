#ifndef LOGGER_H
#define LOGGER_H

#define ARDUINO

#ifdef ARDUINO
#include <Arduino.h>
#include <EEPROM.h>
#endif

#include <stdint.h>
#include <string.h>
#include <time.h>

#define MEMORY_NOT_INITIALIZED	0xff

#define LOG_TYPE	 LOG_T

typedef enum
{
	SAVE_FAILED =   0,
	SAVE_OK,			
	LOAD_FAILED,	    
	LOAD_OK,			 
	CLEAR_OK,		
	CLEAR_FAILED,	
	MAX_ERR_CODE
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
	LOGGER(uint32_t MemorySizeSetted = MEMORY_NOT_INITIALIZED);
	void setMemory(uint32_t MemorySizeSetted);
	uint8_t saveAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t saveSingleData(LogType SingleData);
	uint8_t loadAllData(LogType *AllBuffer, uint32_t BufferSize);
	uint8_t loadLastData(LogType *SingleData);
	uint8_t loadSetData(LogType *Buffer, uint32_t BufferSize, uint32_t SetInit, uint32_t SetEnd);
	uint8_t clearMemory();

private:
	uint32_t MemorySize = MEMORY_NOT_INITIALIZED;
	uint32_t LastDataAddr = 0;
	uint16_t TypeSize;
	bool MemoryFull = false;
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

static EEP<LOG_TYPE> EEPROM(1024);
#endif


template <typename LogType> LOGGER<LogType>::LOGGER(uint32_t MemorySizeSetted)
{
	TypeSize = sizeof(LogType);
	MemorySize = MemorySizeSetted * TypeSize; 
	if(MemorySize > EEPROM.length())
		MemorySize = EEPROM.length();	
}

template <typename LogType> void LOGGER<LogType>::setMemory(uint32_t MemorySizeSetted)
{
	MemorySize = MemorySizeSetted * TypeSize; 
	if(MemorySize > EEPROM.length())
		MemorySize = EEPROM.length();
	clearMemory();
}

template <typename LogType> uint8_t LOGGER<LogType>::saveAllData(LogType *AllBuffer, uint32_t BufferSize)
{
	int Addr = 0;
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return SAVE_FAILED;
	if(BufferSize * TypeSize > MemorySize)
		return SAVE_FAILED;
	if(BufferSize > MemorySetted)
		return SAVE_FAILED;

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
	return SAVE_OK;
}

template <typename LogType> uint8_t LOGGER<LogType>::saveSingleData(LogType SingleData)
{
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return SAVE_FAILED;	
	if(LastDataAddr >= MemorySize)
	{
		LastDataAddr = 0;
		MemoryFull = true;
	}
	EEPROM.put(LastDataAddr, SingleData);
	LastDataAddr += TypeSize;
	return SAVE_OK;
}

template <typename LogType> uint8_t LOGGER<LogType>::loadAllData(LogType *AllBuffer, uint32_t BufferSize)
{
	int Addr = 0;
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return LOAD_FAILED;
	if(BufferSize * TypeSize > MemorySize)
		return LOAD_FAILED;
	if(LastDataAddr == 0)
		return LOAD_FAILED;
	if(BufferSize > MemorySetted)
		return LOAD_FAILED;
	
	for(Addr = 0; Addr < BufferSize; Addr++)
	{
		EEPROM.get((Addr * TypeSize), AllBuffer[Addr]);
	}
	if(MemoryFull)
	{
		LogType Tmp1, Tmp2, Tmp3;
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
	return LOAD_OK;
}

template <typename LogType> uint8_t LOGGER<LogType>::loadLastData(LogType *SingleData)
{
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return LOAD_FAILED;
	if(LastDataAddr >= TypeSize)
		EEPROM.get(LastDataAddr - TypeSize, *SingleData);
	return LOAD_OK;
}

template <typename LogType> uint8_t LOGGER<LogType>::loadSetData(LogType *Buffer, uint32_t BufferSize, uint32_t SetInit, uint32_t SetEnd)
{
	uint32_t MemorySetted = MemorySize / TypeSize;
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return LOAD_FAILED;
	if(BufferSize * TypeSize > MemorySize)
		return LOAD_FAILED;	
    if(SetInit > MemorySetted || SetEnd > MemorySetted)
		return LOAD_FAILED;
	if((SetEnd - SetInit) > BufferSize)
		return LOAD_FAILED;
	for(int Addr = 0; Addr < (SetEnd - SetInit); Addr++)
	{
		EEPROM.get((SetInit * TypeSize) + (Addr * TypeSize), Buffer[Addr]);
	}
	return LOAD_OK;
}

template <typename LogType> uint8_t LOGGER<LogType>::clearMemory()
{
	if(MemorySize == MEMORY_NOT_INITIALIZED)
		return CLEAR_FAILED;
	LogType DataVar;
	memset(&DataVar, 0, TypeSize);
	for(int i = 0; i < MemorySize; i++)
		EEPROM.update(i, DataVar);
	MemoryFull = false;
	return CLEAR_OK;
}

#endif