#pragma once

#include <iostream>
#include <Windows.h>

class CParser
{
public:

	CParser()
		: mFileSize(0)
		, mOffset(0)
		, mpFileData(nullptr)
	{
	}

	~CParser()
	{
		if (mpFileData != nullptr)
		{
			free(mpFileData);
		}
	}

	BOOL LoadFile(const WCHAR* pFileName)
	{
		FILE* fp = nullptr;	

		if (mpFileData != nullptr)
		{
			free(mpFileData);

			clearParser();
		}

		CHAR multiByteFileName[MAX_PATH] = { 0, };
	
		if (WideCharToMultiByte(CP_ACP, 0, pFileName, -1, multiByteFileName, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}

		do
		{
			fopen_s(&fp, multiByteFileName, "r+t");
			if (fp == nullptr)
			{
				return FALSE;
			}

			if (fseek(fp, 0, SEEK_END) != 0)
			{	
				break;
			}

			mFileSize = ftell(fp);

			if (fseek(fp, 0, SEEK_SET) != 0)
			{			
				break;
			}

			mpFileData = (CHAR*)malloc(mFileSize + (INT64)1);
			if (mpFileData == nullptr)
			{	
				break;
			}

			ZeroMemory(mpFileData, mFileSize + (INT64)1);

			if (fread_s(mpFileData, mFileSize + (INT64)1, 1, mFileSize + (INT64)1, fp) == 0)
			{
				break;
			}

			fclose(fp);

			return TRUE;

		} while (0);

		fclose(fp);

		return FALSE;
	}



	BOOL GetValue(const WCHAR* pTag, INT* pValue)
	{
		mOffset = 0;

		CHAR multiByteTag[MAX_PATH] = { 0, };

		if (WideCharToMultiByte(CP_ACP, 0, pTag, -1, multiByteTag, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}

		INT wordLength = 0;

		CHAR retval[MAX_PATH] = { 0, };

		for (;;)
		{		
			if (getNextWord(retval, &wordLength) == FALSE)
			{
				return FALSE;
			}

			if (strcmp(multiByteTag, retval) == 0)
			{
				if (getNextWord(retval, &wordLength) == FALSE)
				{
					return FALSE;
				}

				if (strcmp("=", retval) == 0)
				{	
					if (getNextWord(retval, &wordLength) == FALSE)
					{
						return FALSE;
					}

					*pValue = atoi(retval);

					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
		}
	}

	BOOL GetValue(const WCHAR* pTag, INT64* pValue)
	{
		mOffset = 0;

		CHAR multiByteTag[MAX_PATH] = { 0, };

		if (WideCharToMultiByte(CP_ACP, 0, pTag, -1, multiByteTag, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}

		INT wordLength = 0;

		CHAR retval[MAX_PATH] = { 0, };

		for (;;)
		{
			if (getNextWord(retval, &wordLength) == FALSE)
			{
				return FALSE;
			}

			if (strcmp(multiByteTag, retval) == 0)
			{
				if (getNextWord(retval, &wordLength) == FALSE)
				{
					return FALSE;
				}

				if (strcmp("=", retval) == 0)
				{
					if (getNextWord(retval, &wordLength) == FALSE)
					{
						return FALSE;
					}

					*pValue = atoll(retval);

					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
		}
	}



	BOOL GetString(const WCHAR* pTag, WCHAR* pString, INT stringCchCount)
	{
		ZeroMemory(pString, stringCchCount * sizeof(WCHAR));

		mOffset = 0;

		CHAR multiByteTag[MAX_PATH] = { 0, };

		if (WideCharToMultiByte(CP_ACP, 0, pTag, -1, multiByteTag, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}

		INT wordLength = 0;

		CHAR retval[MAX_PATH] = { 0, };

		for (;;)
		{
			if (getNextWord(retval, &wordLength) == FALSE)
			{
				return FALSE;
			}

			if (strcmp(multiByteTag, retval) == 0)
			{
				if (getNextWord(retval, &wordLength) == FALSE)
				{
					return FALSE;
				}

				if (strcmp("=", retval) == 0)
				{
					if (getNextString(retval, &wordLength) == FALSE)
					{
						return FALSE;
					}

					if (MultiByteToWideChar(CP_ACP, 0, retval, wordLength, pString, stringCchCount) == 0)
					{
						return FALSE;
					}

					return TRUE;
				}
				else
				{
					return FALSE;
				}
			}
		}
	}




	BOOL GetNamespaceValue(const WCHAR* pNamespace, const WCHAR* pTag, INT* pValue)
	{
		mOffset = 0;

		INT wordLength = 0;

		CHAR multiByteNamespace[MAX_PATH] = { 0, };

		CHAR multiByteTag[MAX_PATH] = { 0, };

		CHAR retval[MAX_PATH] = { 0, };

		multiByteNamespace[0] = ':';

		
		if (WideCharToMultiByte(CP_ACP, 0, pNamespace, -1, &multiByteNamespace[1], MAX_PATH - 1, NULL, NULL) == 0)
		{
			return FALSE;
		}

		if (WideCharToMultiByte(CP_ACP, 0, pTag, -1, multiByteTag, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}


		for (;;)
		{
			if (getNextWord(retval, &wordLength) == FALSE)
			{
				return FALSE;
			}

			if (strcmp(multiByteNamespace, retval) == 0)
			{
				if (getNextWord(retval, &wordLength) == FALSE)
				{
					return FALSE;
				}

				if (strcmp(retval, "{") != 0)
				{
					return FALSE;
				}

				for (;;)
				{	
					if (getNextWord(retval, &wordLength) == FALSE)
					{
						return FALSE;
					}
					
					if (strcmp(retval, "}") == 0)
					{
						return FALSE;
					}

					if (strcmp(retval, multiByteTag) == 0)
					{
						if (getNextWord(retval, &wordLength) == FALSE)
						{
							return FALSE;
						}

						if (strcmp(retval, "}") == 0)
						{
							return FALSE;
						}

						if (strcmp(retval,"=") == 0)
						{
							if (getNextWord(retval, &wordLength) == FALSE)
							{
								return FALSE;
							}

							if (strcmp(retval, "}") == 0)
							{
								return FALSE;
							}

							*pValue = atoi(retval);

							return TRUE;
						}
						else
						{
							return FALSE;
						}
					}						
				}
			}
		}
	}



	BOOL GetNamespaceString(const WCHAR* pNamespace, const WCHAR* pTag, WCHAR* pString, INT stringCchCount)
	{
		ZeroMemory(pString, stringCchCount * sizeof(WCHAR));

		mOffset = 0;

		INT wordLength = 0;

		CHAR multiByteNamespace[MAX_PATH] = { 0, };

		CHAR multiByteTag[MAX_PATH] = { 0, };

		CHAR retval[MAX_PATH] = { 0, };

		multiByteNamespace[0] = ':';

		if (WideCharToMultiByte(CP_ACP, 0, pNamespace, -1, &multiByteNamespace[1], MAX_PATH - 1, NULL, NULL) == 0)
		{
			return FALSE;
		}

		if (WideCharToMultiByte(CP_ACP, 0, pTag, -1, multiByteTag, MAX_PATH, NULL, NULL) == 0)
		{
			return FALSE;
		}


		for (;;)
		{
			if (getNextWord(retval, &wordLength) == FALSE)
			{
				return FALSE;
			}

			if (strcmp(multiByteNamespace, retval) == 0)
			{
				if (getNextWord(retval, &wordLength) == FALSE)
				{
					return FALSE;
				}

				if (strcmp(retval, "{") != 0)
				{
					return FALSE;
				}

				for (;;)
				{
					if (getNextWord(retval, &wordLength) == FALSE)
					{
						return FALSE;
					}

					if (strcmp(retval, "}") == 0)
					{
						return FALSE;
					}

					if (strcmp(retval, multiByteTag) == 0)
					{
						if (getNextWord(retval, &wordLength) == FALSE)
						{
							return FALSE;
						}

						if (strcmp(retval, "}") == 0)
						{
							return FALSE;
						}

						if (strcmp(retval, "=") == 0)
						{
							if (getNextString(retval, &wordLength) == FALSE)
							{
								return FALSE;
							}

							if (strcmp(retval, "}") == 0)
							{
								return FALSE;
							}

							if (MultiByteToWideChar(CP_ACP, 0, retval, wordLength, pString, stringCchCount) == 0)
							{
								return FALSE;
							}

							return TRUE;
						}
						else
						{
							return FALSE;
						}
					}
				}
			}
		}
	}



private:

	BOOL IsRemark(void)
	{
		// 주석을 위한 문자 "//" 2BYTE 필요
		if (mOffset >= mFileSize - 1)
		{
			return FALSE;
		}

		if (mpFileData[mOffset] == '/' && mpFileData[mOffset + 1] == '/')
		{

			mOffset += 2;
			
			for (;;)
			{
				if (mOffset >= mFileSize - 1)
				{
					break;
				}
				
				// '\n' 을 만날 때 까지 무시한다.
				if (mpFileData[mOffset] == '\n')
				{
					++mOffset;
					
					return TRUE;
				}
				
				++mOffset;
			}
		}	
		
		return FALSE;
	}

	void skipNoneCommand(void)
	{
		for (;;)
		{
			if (mOffset >= mFileSize - 1)
			{
				break;
			}

			// 0x20 은 스페이스바
			if (mpFileData[mOffset] == '"' || mpFileData[mOffset] == ',' || mpFileData[mOffset] == 0x20 || mpFileData[mOffset] == '\b'
				|| mpFileData[mOffset] == '\t' || mpFileData[mOffset] == '\n' || mpFileData[mOffset] == '\r')
			{

				++mOffset;
			}
			else
			{
				if (IsRemark() == TRUE)
				{
					continue;
				}

				break;
			}
		}

		return;
	}


	void getWordLength(CHAR* pRetval, INT* pWordLength)
	{
		INT wordLength = 0;

		CHAR* pFileData = &mpFileData[mOffset];

		for (;;)
		{		
			// 0x20 은 스페이스바
			if (mpFileData[mOffset] == '"' || mpFileData[mOffset] == ',' || mpFileData[mOffset] == 0x20 || mpFileData[mOffset] == '\b'
				|| mpFileData[mOffset] == '\t' || mpFileData[mOffset] == '\n' || mpFileData[mOffset] == '\r')
			{			

				*pWordLength = wordLength;

				ZeroMemory(pRetval, MAX_PATH);

				memcpy_s(pRetval, MAX_PATH, pFileData, wordLength);

				return;
			}

			++mOffset;
			
			++wordLength;

			if (mOffset >= mFileSize - 1)
			{
				*pWordLength = wordLength;

				ZeroMemory(pRetval, MAX_PATH);

				memcpy_s(pRetval, MAX_PATH, pFileData, wordLength);

				return;
			}
		}
	}


	void getStringLength(CHAR* pRetval, INT* pStringLength)
	{
		INT stringLength = 0;

		CHAR* pFileData = &mpFileData[mOffset];

		for (;;)
		{
			if (mpFileData[mOffset] == '"' || mpFileData[mOffset] == ',' || mpFileData[mOffset] == '\b'
				|| mpFileData[mOffset] == '\t' || mpFileData[mOffset] == '\n' || mpFileData[mOffset] == '\r')
			{

				*pStringLength = stringLength;

				ZeroMemory(pRetval, MAX_PATH);

				memcpy_s(pRetval, MAX_PATH, pFileData, stringLength);

				return;
			}

			++mOffset;

			++stringLength;

			if (mOffset >= mFileSize - 1)
			{
				*pStringLength = stringLength;

				ZeroMemory(pRetval, MAX_PATH);

				memcpy_s(pRetval, MAX_PATH, pFileData, stringLength);

				return;
			}
		}
	}


	// 마지막까지 확인했는데 길이가 0이면은 단어 찾기 실패로 return FALSE
	BOOL getNextWord(CHAR *pRetval,INT* pWordLength)
	{

		// 단어의 시작지점까지 찾는다.
		skipNoneCommand();
		
		getWordLength(pRetval, pWordLength);

		if (*pWordLength == 0 && mOffset >= mFileSize - 1)
		{
			return FALSE;
		}
		
		return TRUE;;
	}


	BOOL getNextString(CHAR* pRetval, INT* pStringLength)
	{
		// 단어의 시작지점까지 찾는다.
		skipNoneCommand();

		getStringLength(pRetval, pStringLength);

		if (*pStringLength == 0 && mOffset >= mFileSize - 1)
		{
			return FALSE;
		}

		return TRUE;;
	}



	void clearParser(void)
	{
		mFileSize = 0;
		mOffset = 0;
		mpFileData = nullptr;
	}
	
	INT mFileSize;

	INT mOffset;

	CHAR* mpFileData;
};

