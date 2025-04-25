#include <iostream>

#include "system.h"

int main()
{
	cSystem system; 

	system.Initialize();

	while (true)
	{
		system.Run();
	}

	system.Finalize(); 
}