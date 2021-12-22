#include "Logger.h"

LOGGER<LOG_TYPE> Log;


void setup()
{
	LOG_TYPE pero;
	Log.begin(10);
	Log.manageDir("rootA", true);
}


void loop()
{

}