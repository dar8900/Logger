#include "Logger.h"

LOGGER<LOG_TYPE> Log;


void setup()
{
	LOG_TYPE pero;
	Log.begin(20);
	Log.saveSingleData(pero);
	Log.loadLastData(&pero);
}


void loop()
{

}