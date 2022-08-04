#include "stdafx.h"
#include "CJumpPointSearch.h"
#include "JumpPointSearchAlgorithm.h"
#include "BresenhamLine.h"


bool BresenhamLine::MakeLine(int startX, int startY, int endX, int endY)
{
	int subX = abs(startX - endX);
	int subY = abs(startY - endY);

	int addX = 0;
	int addY = 0;

	int errorValue;

	int indexX;
	int indexY;

	if (startX <= endX)
	{
		indexX = startX + addX;
	}
	else
	{
		indexX = startX - addX;
	}

	if (startY <= endY)
	{
		indexY = startY + addY;
	}
	else
	{
		indexY = startY - addY;
	}

	if (subX >= subY)
	{
		// 부자연스러운 연결을 막기위해 errorValue 값을 셋팅
		errorValue = subX / 2;

		for(;;)
		{
			if (subX == addX && subY == addY)
			{
				break;
			}

			if (brushBlockList[indexY][indexX] == grayBrush)
			{
				return false;
			}

			addX += 1;
			if (startX <= endX)
			{
				indexX = startX + addX;
			}
			else
			{
				indexX = startX - addX;
			}

			errorValue += subY;
			if (subX <= errorValue)
			{
				addY += 1;
				if (startY <= endY)
				{
					indexY = startY + addY;
				}
				else
				{
					indexY = startY - addY;
				}

				errorValue -= subX;
			}
		}
	}
	else
	{
		errorValue = subY / 2;

		for(;;)
		{
			if (subX == addX && subY == addY)
			{
				break;
			}

			if (brushBlockList[indexY][indexX] == grayBrush)
			{
				return false;
			}

			addY += 1;
			if (startY <= endY)
			{
				indexY = startY + addY;
			}
			else
			{
				indexY = startY - addY;
			}

			errorValue += subX;
			if (subY <= errorValue)
			{
				addX += 1;
				if (startX <= endX)
				{
					indexX = startX + addX;
				}
				else
				{
					indexX = startX - addX;
				}

				errorValue -= subY;
			}
		}
	}

	return true;
}


bool BresenhamLine::CatchLine(int startX, int startY, int endX, int endY)
{
	int subX = abs(startX - endX);
	int subY = abs(startY - endY);

	int addX = 0;
	int addY = 0;

	int errorValue;

	int indexX;
	int indexY;

	if (startX <= endX)
	{
		indexX = startX + addX;
	}
	else
	{
		indexX = startX - addX;
	}

	if (startY <= endY)
	{
		indexY = startY + addY;
	}
	else
	{
		indexY = startY - addY;
	}

	if (subX >= subY)
	{
		errorValue = subX / 2;

		for(;;)
		{
			if (subX == addX && subY == addY)
			{
				break;
			}

			if (brushBlockList[indexY][indexX] != grayBrush && brushBlockList[indexY][indexX] != greenBrush && brushBlockList[indexY][indexX] != redBrush && brushBlockList[indexY][indexX] != yellowBrush)
			{
				brushBlockList[indexY][indexX] = routeBrush;
			}
			else if (brushBlockList[indexY][indexX] == grayBrush)
			{
				return false;
			}

			addX += 1;
			if (startX <= endX)
			{
				indexX = startX + addX;
			}
			else
			{
				indexX = startX - addX;
			}

			errorValue += subY;
			if (subX <= errorValue)
			{
				addY += 1;
				if (startY <= endY)
				{
					indexY = startY + addY;
				}
				else
				{
					indexY = startY - addY;
				}

				errorValue -= subX;
			}
		}
	}
	else
	{
		errorValue = subY / 2;

		for(;;)
		{
			if (subX == addX && subY == addY)
			{
				break;
			}

			if (brushBlockList[indexY][indexX] != grayBrush && brushBlockList[indexY][indexX] != greenBrush && brushBlockList[indexY][indexX] != redBrush && brushBlockList[indexY][indexX] != yellowBrush)
			{
				brushBlockList[indexY][indexX] = routeBrush;
			}
			else if (brushBlockList[indexY][indexX] == grayBrush)
			{
				return false;
			}
			
			addY += 1;
			if (startY <= endY)
			{
				indexY = startY + addY;
			}
			else
			{
				indexY = startY - addY;
			}

			errorValue += subX;
			if (subY <= errorValue)
			{
				addX += 1;
				if (startX <= endX)
				{
					indexX = startX + addX;
				}
				else
				{
					indexX = startX - addX;
				}

				errorValue -= subY;
			}
		}
	}

	return true;
}
