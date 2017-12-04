//---------------------------------------------------------------------------
#include <vcl\condefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma hdrstop

//---------------------------------------------------------------------------
//USERES("test.res");
int main2(int argc, char **argv);  // BORL main2
USEUNIT("comptest.cpp");
//---------------------------------------------------------------------------

int main(int argc, char **argv)
{
	return main2(argc, argv);
}
//---------------------------------------------------------------------------
