#include <iostream>

#include "system.h"

int main()
{
	cSystem system; 

	system.Initialize();

	system.Run();

	system.Finalize(); 

	return 0; 
}