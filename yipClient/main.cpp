#include "yip.h"

#include "types.h"

#include <tchar.h>

using namespace uti;

int _tmain(int argc, _TCHAR* argv[])
{
	int32 iReturn = yip::clientFunc(nullptr);

	system("PAUSE");

	return iReturn;
}
