//PassMark MemTest86
//
//Copyright (c) 2014-2016
//	This software is the confidential and proprietary information of PassMark
//	Software Pty. Ltd. ("Confidential Information").  You shall not disclose
//	such Confidential Information and shall use it only in accordance with
//	the terms of the license agreement you entered into with PassMark
//	Software.
//
//Program:
//	MemTest86
//
//Module:
//	LineChart.h
//
//Author(s):
//	Keith Mah
//
//Description:
//	A class for dislaying line charts in windows. Can handle multiple series,
//	ledgends and scaling of the X and Y axis.
//
//  Taken from PerformanceTest
//
//History
//	15 Jul 2014: Initial version

#ifndef _LINECHART_H_
#define _LINECHART_H_

#define MAX_LINE_POINTS		32	//Max number points in a series.


#define MAX_NUMSERIES		10		//Max number of series that can be displayed

//Possbile properties
#define	YAXISUNITS			1

//Possible YAXISUNITS values
#define YAXISUNITS_MINUTES	1
#define YAXISUNITS_INTEGER	2


void LineChart_Init();
void LineChart_Destroy();

int		LineChart_GetGraphHeight ();
void	LineChart_SetChartTitle(CHAR16 * szNewTitle);
CHAR16*	LineChart_GetChartTitle();
void	LineChart_SetXAxisText(CHAR16 * szNewText);
int		LineChart_SetProperty (int Property, int NewValue);
int		LineChart_SetCatTitle (int Column, CHAR16 *Title);
CHAR16*	LineChart_GetCatTitle (int Column);
int		LineChart_SetSerTitle (int Series, CHAR16 *Title);
int		LineChart_SetSerSubtitle (int Series, CHAR16 *Title);
CHAR16*	LineChart_GetSerTitle (int Series);
CHAR16*	LineChart_GetSerSubtitle (int Series);
int		LineChart_SetCatValue (int Series, int Cat, INT64 Value);
INT64	LineChart_GetCatValue (int Series, int Cat);
int		LineChart_SetNumCat (int Series, int NCat);
int		LineChart_GetNumCat (int Series);
int		LineChart_GetMaxNumCat ();
int		LineChart_SetContValue (int Series, int PointIdx, INT64 Xvalue, INT64 Yvalue);
INT64	LineChart_GetContXValue (int Series, int PointIdx);
INT64	LineChart_GetContYValue (int Series, int PointIdx);
INT64	LineChart_GetMaxYScale ();
int		LineChart_SetNumPoints (int Series, int NumPoints);
int		LineChart_GetNumPoints (int Series);
void	LineChart_SetChartWidth (int NewWidth);
void	LineChart_SetChartHeight (int NewHeight);
int		LineChart_GetChartWidth (void);
int		LineChart_GetChartHeight (void);
int		LineChart_GetFullChartWidth (void);
int		LineChart_GetFullChartHeight (void);
int		LineChart_SetNumSer(int NSer);
int		LineChart_GetNumSer();

void	LineChart_SetDiscrete();
BOOLEAN	LineChart_IsDiscrete();
void	LineChart_ShowDataPoints(int Series, BOOLEAN bShow);
void	LineChart_SetSeriesWidth(int Series, int iPixels);
int		LineChart_GetSeriesWidth(int Series);
void	LineChart_SetSeriesColour(int Series, EG_PIXEL *colour);
EG_PIXEL LineChart_GetSeriesColour(int Series);
BOOLEAN	LineChart_IsShowDataPoints(int Series);
void	LineChart_ShowGrid(BOOLEAN bShow);
BOOLEAN	LineChart_IsShowGrid();
void	LineChart_SetGridThickness(int iPixels);
int		LineChart_GetGridThickness();
void	LineChart_SetBackgroundColour(EG_PIXEL *color1, EG_PIXEL *color2, BOOLEAN bGradient);
BOOLEAN	LineChart_SetBackgroundImage(CHAR16 * szFilePath);
void	LineChart_SetAltBGOverlay(BOOLEAN bShow);
	
void	LineChart_SetAltBGOverlay(BOOLEAN bShow);
BOOLEAN	LineChart_GetAltBGOverlay();

int		LineChart_DisplayChart (EG_IMAGE *Image, int x, int y);

#endif