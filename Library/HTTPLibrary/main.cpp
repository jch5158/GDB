#include "CHTTPPost.h"
#include "library/Dump/CCrashDump.h"

using namespace shin;

int main()
{
	CHTTPPost http(1, 1000, 1000);
	http.SetServerURL(L"10.0.0.1/login.php");

	char test[] = "{\"id\" : \"ID_0\",\"password\" : \"procademy\"}";
	
	int recv = 0;
	CHAR buffer[1024] = { 0 };
	http.POST(nullptr, test, buffer, 1024, &recv);

	printf("%s", buffer);

	system("pause");
}