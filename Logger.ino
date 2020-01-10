#include "Logger.h"

LOGGER<String> Log(1024);


void setup()
{
	String pero = "ciaone";
	Log.saveSingleData(pero);
	Log.loadLastData(&pero);
}


void loop()
{

}