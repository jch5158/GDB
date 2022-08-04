#include "stdafx.h"

CTLSDBConnector dbConnector;

void DBWriteFunction(void)
{
	if (dbConnector.Query((WCHAR*)L"begin;") == FALSE)
	{
		wprintf_s(L"begin failed, error no : %d, error msg : %s\n", dbConnector.GetLastError(), dbConnector.GetLastErrorMessage());
	}

	if (dbConnector.Query((WCHAR*)L"UPDATE `character` SET `level` = `level` + 1 WHERE `character_no` = %lld;", 1) == FALSE)
	{
		wprintf_s(L"levelup failed, error no : %d, error msg : %s\n", dbConnector.GetLastError(), dbConnector.GetLastErrorMessage());
	}

	if (dbConnector.Query((WCHAR*)L"commit;") == FALSE)
	{
		wprintf_s(L"commit failed, error no : %d, error msg : %s\n", dbConnector.GetLastError(), dbConnector.GetLastErrorMessage());
	}

	if (dbConnector.Query((WCHAR*)L"SELECT * FROM `character`;") == FALSE)
	{
		wprintf_s(L"commit failed, error no : %d, error msg : %s\n", dbConnector.GetLastError(), dbConnector.GetLastErrorMessage());
	}

	MYSQL_ROW mysqlRow = dbConnector.FetchRow();

	dbConnector.FreeResult();

	printf_s("no : %s, level : %s, money : %s\n", mysqlRow[0], mysqlRow[1], mysqlRow[2]);

	return;
}


INT main()
{
 //   CTLSDBConnector DBConnector;

 //   if (DBConnector.Connect("") == false)
 //   {
 //       wprintf_s(L"connect failed, Error Code : %d, Error Msg : %s\n", DBConnector.GetLastError(), DBConnector.GetLastErrorMessage());

 //       return 1;
 //   }

 //   if (DBConnector.Query(L"SELECT * FROM account where account_no = 2;") == TRUE)
 //       std::wcout << "Success\n";
 //   else
 //   {
 //       std::wcout << "Failed\n";
 //           
 //       return 1;
 //   }

 //   DBConnector.StoreResult();

 //   MYSQL_ROW mysqlRow = DBConnector.FetchRow();
 //   if (mysqlRow == nullptr)
 //   {
 //       std::wcout << L"mysqlRow is nullptr\n";

 //       return 1;
 //   }

 //   if (mysqlRow[0] != nullptr)
 //       std::cout << mysqlRow[0] << " ";
 //   
	//if (mysqlRow[1] != nullptr)
	//	std::cout << mysqlRow[1] << " ";

	//if (mysqlRow[3] != nullptr)
	//	std::cout << mysqlRow[3] << "\n";

 //   DBConnector.FreeResult();

 //   DBConnector.Disconnect();

 //   system("pause");

    return 1;
}
