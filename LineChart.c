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
//	LineChart.c
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

#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PrintLib.h>

#include <Library/MemTestUiLib.h>
#include <Library/libeg/libeg.h>

#include "LineChart.h"

#define STRING_LEN (32 * sizeof(CHAR16))
#define TITLE_STRING_LEN (64 * sizeof(CHAR16))

typedef struct {
	INT64 x;
	INT64 y;
} LCPOINT;


STATIC int		NumHorzDiv;
STATIC int		NumXAxisTextLables;
STATIC int		YAxisUnits;
STATIC int		CatGap;
STATIC int		CatWidthTotal;						//Width of each category. Float stops rounding errors
STATIC INT64	MaxYScale;
STATIC INT64	MaxXScale;
STATIC int		MaxNumCat;
STATIC void		CalcScale ();

STATIC CHAR16	* szSerTitles[MAX_NUMSERIES];				//Array of pointers to title strings
STATIC CHAR16	* szSerSubtitles[MAX_NUMSERIES];			//Array of pointers to subtitle strings
STATIC int		NumSer;										//The number of series/lines to display.

STATIC INT64	CatValues[MAX_NUMSERIES][MAX_LINE_POINTS];	//The integer values for each series / line (discreet x-axis)
STATIC CHAR16	* szCatTitles[MAX_LINE_POINTS];				//Array of pointers to title strings
STATIC int		NumCat[MAX_NUMSERIES];						//The number of categories to display on the X axis. (discreet x-axis)

STATIC LCPOINT	ContPoints[MAX_NUMSERIES][MAX_LINE_POINTS]; //The integer values for each series / line (continuous x-axis)
STATIC int		NumContPoints[MAX_NUMSERIES];				//The number of points (continous x-axis)

STATIC CHAR16	* szChartTitle;
STATIC CHAR16	* szXAxisText;
STATIC int		ChartWidth;						//Chart width
STATIC int		ChartHeight;
STATIC int		LegendGap;
STATIC int		LegendBoxWidth;					//Width of Ledgend box
STATIC int		LegendBoxHeight;				//Height of Ledgend box
STATIC int		LegendBoxGap;					//Gap between Ledgend boxes
STATIC int		TitleHeight;

STATIC EG_PIXEL	BackgroundColour1;				//0x00bbggrr
STATIC EG_PIXEL	BackgroundColour2;				//Optional BG colour for gradient
STATIC BOOLEAN		m_bGradient;					// Enable vertical gradient from BackgroundColour1 to BackgroundColour2
STATIC int		AxisWidth;
	// COLORREF	DivLineColour;

STATIC  BOOLEAN	m_bShowDataPoints[MAX_NUMSERIES];	//Show/hide data points in series
STATIC int		m_iSeriesThickness[MAX_NUMSERIES];	//Series line thickness in pixels
STATIC EG_PIXEL m_seriesColour[MAX_NUMSERIES];		//Series line colour
STATIC BOOLEAN	m_bShowGrid;						//Show/hide grid
STATIC int		m_iGridThickness;					//Grid thickness in pixels

//	HBITMAP m_DIBBitmap;						//bitmap for chart background
STATIC BOOLEAN m_bAltOverlay;							//enable/disable alternating semi-transparent overlay

STATIC BOOLEAN	m_bDiscrete;

const int	SeriesColours[MAX_NUMSERIES][3]= {
	{255,0,0},		//0
	{0,255,0},
	{0, 0,255},
	{255,255,0},
	{0,255,255},
	{127,0,0},
	{127,127,0},
	{0,127,0},
	{0,127,127},
	{0, 0,127},
/*	255,0,255,
	128,0,0,		
	128,128,0,
	0,128,0,
	0,128,128,		//9
	0, 0,128,		//10
	128,0,128,
	255,128,128,
	255,255,128,
	128,255,128,
	128,255,255,
	128, 128,255,
	255,128,255,
	192,192,192,
	255,159,9,		//19
*/
};

static EFI_GRAPHICS_OUTPUT_BLT_PIXEL BlackColour = { 0, 0, 0, 0};
static EFI_GRAPHICS_OUTPUT_BLT_PIXEL WhiteColour = { 255, 255, 255, 255};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void LineChart_Init()
{
	// LOGFONT			lf;
	int				i;

	SetMem(szCatTitles, sizeof(szCatTitles), 0);
	SetMem(CatValues, sizeof(CatValues), 0);
	SetMem(ContPoints, sizeof(ContPoints), 0);
	SetMem(szSerTitles, sizeof(szSerTitles), 0);
	SetMem(szSerSubtitles, sizeof(szSerSubtitles), 0);
	SetMem(szSerSubtitles, sizeof(szSerSubtitles), 0);
	SetMem(m_bShowDataPoints, sizeof(m_bShowDataPoints), 0);
	SetMem(NumContPoints, sizeof(NumContPoints), 0);
	SetMem(NumCat, sizeof(NumCat), 0);

	for (i = 0; i < MAX_NUMSERIES; ++i)
	{
		m_iSeriesThickness[i] = 1;
		m_seriesColour[i].r = (UINT8)SeriesColours[i][0];
		m_seriesColour[i].g = (UINT8)SeriesColours[i][1];
		m_seriesColour[i].b = (UINT8)SeriesColours[i][2];
	}

	szChartTitle	= NULL;
	szXAxisText		= NULL;

	//Set default values
	ChartWidth		= 400;		//Width in pixels of the whole chart including the Y Axis Text etc.
	ChartHeight		= 350;		//Height in pixels of the whole chart including the X Axis Text etc.
								//but does not include the Main Title (see TitleHeight below)
	NumSer			= 0;		//Number of series
	LegendGap		= 10;		//Gap between ledgend and the chart
	LegendBoxWidth	= 30;		//Width of Ledgend box
	LegendBoxHeight	= 30;		//Height of Ledgend box
	LegendBoxGap	= 10;		//Gap between Ledgend boxes
	TitleHeight		= 40;		//Height of space for title text
	CatGap			= 2;		//Gap between categories / columns (used for text)
	CatWidthTotal	= 20;		//Default column width, will vary as columns are added
	YAxisUnits		= YAXISUNITS_INTEGER;
	AxisWidth		= 3;		//Width of X and Y axis
	NumXAxisTextLables = 9;
	BackgroundColour1.r = 0xC8;
	BackgroundColour1.g = 0xC8;
	BackgroundColour1.b = 0xC8;
	BackgroundColour2.r  = 255;
	BackgroundColour2.g = 255;
	BackgroundColour2.b = 255;
	m_bGradient		= FALSE;
	NumHorzDiv		= 10;			//The number of horizontal divisions (lines)
	// DivLineColour	= RGB(192,192,192);	//Note clear dots are white

	m_bShowGrid = FALSE;
	m_iGridThickness = 1;
	//m_DIBBitmap = NULL;
	m_bAltOverlay = FALSE;

	m_bDiscrete = FALSE;

}


void LineChart_Destroy()
{
	int i;
	for (i = 0; i < MAX_LINE_POINTS; ++i) {
		if (szCatTitles[i] != NULL)
			FreePool(szCatTitles[i]);
	}
	for (i = 0; i < MAX_NUMSERIES; ++i) {
		if (szSerTitles[i] != NULL)
			FreePool (szSerTitles[i]);
		if (szSerSubtitles[i] != NULL)
			FreePool  (szSerSubtitles[i]);
	}

	if (szChartTitle != NULL)
		FreePool  (szChartTitle);
	if (szXAxisText != NULL)
		FreePool  (szXAxisText);
}


// Name: SetNumCat
// Function: 
//	Sets the number of categories in the line chart (discreet x-axis)
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN NCat, number of categories
int LineChart_SetNumCat(int Series, int NCat)
{
	int RetVal = 0;
	if (Series >= MAX_NUMSERIES)
		return 2;
	if (NCat > MAX_LINE_POINTS) {
		NCat = MAX_LINE_POINTS ;
		RetVal = 1;
	}
	NumCat[Series] = NCat;
	CalcScale();
	return RetVal;
}

int		LineChart_GetNumCat (int Series)
{
	return NumCat[Series];
}

int		LineChart_GetMaxNumCat ()
{
	return MaxNumCat;
}

// Name: SetNumPoints
// Function: 
//	Sets the number of points in the line chart (continuous x-axis)
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN NumPoints, number of points
int LineChart_SetNumPoints(int Series, int NumPoints)
{
	int RetVal = 0;

	if (Series >= MAX_NUMSERIES)
		return 2;
	if (NumPoints > MAX_LINE_POINTS) {
		NumPoints = MAX_LINE_POINTS ;
		RetVal = 1;
	}

	NumContPoints[Series] = NumPoints;
	CalcScale();
	return RetVal;
}

int		LineChart_GetNumPoints (int Series) { 
	return NumContPoints[Series]; 
}

void	LineChart_SetChartWidth (int NewWidth)	
{ 
	ChartWidth  = NewWidth ;
}

void	LineChart_SetChartHeight (int NewHeight)	{ 
	ChartHeight = NewHeight ;
}

int		LineChart_GetChartWidth (void)  
{ 
	return ChartWidth; 
}

int		LineChart_GetChartHeight (void) { 
	return ChartHeight;
}

int		LineChart_GetFullChartWidth (void)
{
	int i;
	UINTN MaxTextWidth = 0;
	for (i = 0; i < NumSer; ++i)
	{
		if (szSerTitles[i])
		{
			UINTN TextWidth, TextHeight;
			GetTextInfo(szSerTitles[i], &TextWidth, &TextHeight);

			if (TextWidth > MaxTextWidth)
				MaxTextWidth = TextWidth;

			if (szSerSubtitles[i])
			{
				GetTextInfo(szSerSubtitles[i], &TextWidth, &TextHeight);

				if (TextWidth > MaxTextWidth)
					MaxTextWidth = TextWidth;
			}
		}
	}
	return ChartWidth + LegendGap + LegendBoxWidth + LegendBoxGap/2 + (int)MaxTextWidth;
}

int		LineChart_GetFullChartHeight (void)
{
	return ChartHeight + TitleHeight;
}

// Name: SetNumSer
// Function: 
//	Sets the number of series / lines to display
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN NSer, number of series
int LineChart_SetNumSer(int NSer)
{
	if (NSer > MAX_NUMSERIES) 
		NumSer = MAX_NUMSERIES;
	NumSer = NSer;
	CalcScale();
	return 0;
}

int		LineChart_GetNumSer() { 
	return NumSer; 
}

// Name: SetCatValue
// Function: 
//	Sets the value of one particular element for 1 series for 1 category (discreet x-axis)
//	Note that categories are numbered from 0
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN Series,	Series number ( 0 - MAX_NUMSERIES)
//	IN Cat,		Category (0 - MAX_LINE_POINTS)
//	IN Value,	New value
int LineChart_SetCatValue(int Series, int Cat, INT64 Value)
{
	if (Cat >= MAX_LINE_POINTS)
		return 1;
	if (Series >= MAX_NUMSERIES)
		return 2;
	CatValues[Series][Cat] = Value;
	CalcScale ();
	return 0;
}

INT64	LineChart_GetCatValue (int Series, int Cat)
{
	return CatValues[Series][Cat];
}

// Name: SetContValue
// Function: 
//	Sets the coordinates for 1 point in 1 series (continuous x-axis)
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN Series,	Series number ( 0 - MAX_NUMSERIES)
//	IN PointIdx Point number (0 - MAX_LINE_POINTS)
//  IN Xvalue	x-coordinate value
//	IN Yvalue,	y-coordinate value
int	LineChart_SetContValue (int Series, int PointIdx, INT64 Xvalue, INT64 Yvalue)
{
	if (PointIdx >= MAX_LINE_POINTS)
		return 1;
	if (Series >= MAX_NUMSERIES)
		return 2;
	ContPoints[Series][PointIdx].x = Xvalue;
	ContPoints[Series][PointIdx].y = Yvalue;
	CalcScale ();
	return 0;
}

INT64		LineChart_GetContXValue (int Series, int PointIdx) { 
	return ContPoints[Series][PointIdx].x; 
}

INT64		LineChart_GetContYValue (int Series, int PointIdx) { 
	return ContPoints[Series][PointIdx].y; 
}


// Name: SetCatTitle
// Function: 
//	Sets a category title (discrete x-axis)
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN Cat,		category
//	IN Value,	New category value
int LineChart_SetCatTitle(int Cat, CHAR16 * szTitle)
{
	if (Cat >= MAX_LINE_POINTS)
		return 1;

	//Allocate memory for title if required
	if (szCatTitles[Cat] == NULL)
		szCatTitles[Cat] = (CHAR16 *) AllocatePool (STRING_LEN);

	StrCpyS(szCatTitles[Cat], STRING_LEN / sizeof(CHAR16), szTitle);
	return 0;
}

CHAR16*	LineChart_GetCatTitle (int Cat)
{
	return szCatTitles[Cat];
}


// Name: SetSerTitle
// Function: 
//	Sets the title for a series
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN Series,	Series number (starts at 0)
//	IN szTitle,	Series text, , used in ledgend
int LineChart_SetSerTitle(int Series, CHAR16 * szTitle)
{
	if (Series >= MAX_NUMSERIES)
		return 1;

	//Allocate memory for title if required
	if (szSerTitles[Series] == NULL)
		szSerTitles[Series] = (CHAR16 *) AllocatePool (STRING_LEN);

	StrCpyS(szSerTitles[Series], STRING_LEN / sizeof(CHAR16), szTitle);
	return 0;
}

CHAR16*	LineChart_GetSerTitle (int Series) { 
	return szSerTitles[Series]; 
}

CHAR16*	LineChart_GetSerSubtitle (int Series) {
	return szSerSubtitles[Series];
}

int	LineChart_SetSerSubtitle (int Series, CHAR16 *szTitle)
{
	if (Series >= MAX_NUMSERIES)
		return 1;

	//Allocate memory for title if required
	if (szSerSubtitles[Series] == NULL)
		szSerSubtitles[Series] = (CHAR16 *) AllocatePool (STRING_LEN);

	StrCpyS(szSerSubtitles[Series], STRING_LEN / sizeof(CHAR16), szTitle);
	return 0;
}

// Name: SetBackgroundColour
// Function: 
//	Sets the background colour for the chart (and optinally a gradient)
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN color1,	the RGB value for the background colour
//	IN color2,	the RGB value for an optional second colour for a gradient background
//	IN bGradient, set to TRUE to use a gradient background. Otherwise, use color1 as the background colour
void LineChart_SetBackgroundColour(EG_PIXEL *color1, EG_PIXEL *color2, BOOLEAN bGradient)
{
	if (color1) CopyMem(&BackgroundColour1, color1, sizeof(BackgroundColour1));
	if (color2) CopyMem(&BackgroundColour2, color2, sizeof(BackgroundColour2));
	m_bGradient = bGradient;
}

void	LineChart_SetDiscrete() { m_bDiscrete = TRUE; }
BOOLEAN	LineChart_IsDiscrete() { return m_bDiscrete; }
void	LineChart_ShowDataPoints(int Series, BOOLEAN bShow) { m_bShowDataPoints[Series] = bShow; }
void	LineChart_SetSeriesWidth(int Series, int iPixels) { m_iSeriesThickness[Series] = iPixels; }
int		LineChart_GetSeriesWidth(int Series) { return m_iSeriesThickness[Series]; }
void	LineChart_SetSeriesColour(int Series, EG_PIXEL *colour) { CopyMem(&m_seriesColour[Series],colour,sizeof(m_seriesColour[Series])); }
EG_PIXEL LineChart_GetSeriesColour(int Series) { return m_seriesColour[Series]; }
BOOLEAN	LineChart_IsShowDataPoints(int Series) { return m_bShowDataPoints[Series]; }
void	LineChart_ShowGrid(BOOLEAN bShow) { m_bShowGrid = bShow; }
BOOLEAN	LineChart_IsShowGrid() { return m_bShowGrid; }
void	LineChart_SetGridThickness(int iPixels) { m_iGridThickness = iPixels; }
int		LineChart_GetGridThickness() { return m_iGridThickness; }

void	LineChart_SetAltBGOverlay(BOOLEAN bShow) { m_bAltOverlay = bShow; }
BOOLEAN	LineChart_GetAltBGOverlay() { return m_bAltOverlay; }

// Name: DisplayChart
// Function: 
//	Display the chart
// Returns: 
//	0 if OK
//	!0 if error
// Parameters:
//	IN hdc, Device context
//	IN x,y,	Upper left hand corner of the chart
int LineChart_DisplayChart(EG_IMAGE *Image, int x, int y)
{
	int			i;
	int			iSer;
	EG_PIXEL	SColour;			//Segment colour
	int			LegendTop, LegendLeft;
	int			ChartTop, ChartLeft;
	int			BoxULx, BoxULy;		//Upper left corner of Ledgend box or other element
	int			BoxLRx, BoxLRy;		//Lower right corner of Ledgend box or other element
	int			YAxisTopx,YAxisTopy;
	int			YAxisHeight;
	int			YAxisBottomx,YAxisBottomy;
	int			XAxisLeftx,XAxisLefty;
	int			XAxisRightx,XAxisRighty;
	int			XAxisWidth;
	CHAR16		ValStr[20];
	int			StrLength;
	int			MaxEnt;				//Width of longest text string
	int			ptx, pty;
	UINTN		TextWidth, TextHeight;
	int			VisChartHeight;		//Visible chart height
	int			TotalCatGap;

	int			j, k;
	int			f;
	INT64		tempCat;
	
	struct {
		int left;
		int top;
		int right;
		int bottom;
	} TextRec;

	int			MaxColTextHeight	= 0;
	int			LabelSkip			= 0;

	int			DivSizeUnits	= 0;
	int			DivSizePixels;
	int			DivY;

	int		DivXSizePixels;

	EG_PIXEL GridColour = { 127, 127, 127, 127 };

	//Draw background
	egFillArea( Image, &BackgroundColour1, x, y, LineChart_GetFullChartWidth(), LineChart_GetFullChartHeight());

	//
	//Display the title text, Note that the title is assumes to be outside of the
	//actual chart area.
	//
	if (szChartTitle != NULL) 
	{
		ImagePrintXYAlign( Image, x, y, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, szChartTitle);
	}


	//
	// Display maximum Y Scale value
	//

	ChartTop = y + TitleHeight;
	ChartLeft = x;

	if(MaxYScale > 10)
	{
		UnicodeSPrint(ValStr, sizeof(ValStr), L"%ld", MaxYScale);
	}
	else
	{
		UnicodeSPrint(ValStr, sizeof(ValStr), L"%ld.%1d", DivU64x32(MaxYScale, 10), ModU64x32(MaxYScale, 10));
	}

	ImagePrintXYAlign( Image, ChartLeft, ChartTop, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, ValStr);
	GetTextInfo(ValStr, &TextWidth, &TextHeight);
	

	YAxisTopx = ChartLeft + (int)TextWidth + 5;


	//
	//Display X axis text (usually units of measurement)
	//
	if (szXAxisText != NULL)
	{
		int	TextXPos, TextYPos;
		GetTextInfo(szXAxisText, &TextWidth, &TextHeight);
		TextXPos = ChartLeft + (ChartWidth - (int)TextWidth) / 2;
		TextYPos = ChartTop + ChartHeight - (int)TextHeight;
		ImagePrintXYAlign( Image, TextXPos, TextYPos, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, szXAxisText);

		VisChartHeight = ChartHeight - (int)TextHeight - 8;
	}
	else
	{
		VisChartHeight = ChartHeight;
	}

	//
	// Calculate height required for x-axis text
	//
	if (!LineChart_IsDiscrete()) // Continuous x-axis
	{
		UnicodeSPrint(ValStr, sizeof(ValStr), L"%d.%1d", MaxXScale);
		GetTextInfo(ValStr, &TextWidth, &TextHeight);
		MaxColTextHeight = (int)TextWidth;
	}
	else // Discrete x-axis
	{
		//
		//Calculate column width
		//
		TotalCatGap = (MaxNumCat+1) * CatGap;
		if (TotalCatGap >= (((ChartWidth-(YAxisTopx - x))*CatGap)/(CatGap+1))+CatGap)
			TotalCatGap =  (((ChartWidth-(YAxisTopx - x))*CatGap)/(CatGap+1))+CatGap;
		if (MaxNumCat != 0)
			CatWidthTotal = (ChartWidth - (YAxisTopx - x) - TotalCatGap); // / MaxNumCat;
		else
			CatWidthTotal = 1;
		if (CatWidthTotal <= 0)
			CatWidthTotal = 1;

		// catsPerPixel = 1/CatWidth;

		//
		//Display X axis text for each column. Note: Text is at 90 degress
		//			

		if(MaxNumCat > 15)
		{
			LabelSkip = MaxNumCat / NumXAxisTextLables;
		}
		else
		{
			LabelSkip = 1;
		}

		//First, get max text height
		GetTextInfo(szCatTitles[0], &TextWidth, &TextHeight);
		MaxColTextHeight = (int)TextHeight;

		//Now display text
		TextRec.top = ChartTop + VisChartHeight - MaxColTextHeight;
		TextRec.bottom = ChartTop + VisChartHeight + 5;
		for (i = 0; i < MaxNumCat; i += LabelSkip) {
			TextRec.left = (i * CatWidthTotal / MaxNumCat) + CatGap + YAxisTopx;
			if (CatWidthTotal < MaxNumCat)
				TextRec.left += (i * CatGap * CatWidthTotal / MaxNumCat);
			else
				TextRec.left += (i * CatGap);

			GetTextInfo(szCatTitles[i], &TextWidth, &TextHeight);
			TextRec.left -= (int)TextWidth/2;

			TextRec.right = TextRec.left + (LabelSkip * CatWidthTotal / MaxNumCat );

			ImagePrintXYAlign( Image, TextRec.left, TextRec.top, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, szCatTitles[i]);
		}
	}

	//
	//Display background
	//
	YAxisTopy = ChartTop;
	YAxisBottomx = YAxisTopx;
	YAxisBottomy = YAxisTopy + VisChartHeight - MaxColTextHeight - 2;
	XAxisLeftx = YAxisBottomx;
	XAxisLefty = YAxisBottomy;
//	XAxisRightx = XAxisLeftx + ChartWidth;	Modified 1/7/2002
	XAxisRightx = ChartWidth + ChartLeft;
	XAxisRighty = XAxisLefty;

	if (m_bGradient) // Check if using gradient background
	{
	}
	else // solid colour backgorund
	{
	}

	//
	// Calculate x-axis/y-axis divisions for grid/labels
	//
	if(MaxYScale > NumHorzDiv)
	{
		DivSizeUnits = (int)(DivU64x32(MaxYScale,NumHorzDiv));
	}

	DivSizePixels = (YAxisBottomy - YAxisTopy) / NumHorzDiv;

	DivXSizePixels = (XAxisRightx - XAxisLeftx) / NumXAxisTextLables;

	if (m_bAltOverlay) // semi-transparent overlay
	{
		EG_PIXEL SemiTransColour = { 255, 255, 255, 64 };
		
		for (i = 1; i <= NumHorzDiv; i+=2) {
			DivY = YAxisBottomy - i * DivSizePixels;
			egClearScreenArea( &SemiTransColour, XAxisLeftx, DivY, XAxisRightx - XAxisLeftx, DivSizePixels);
		}
	}

	//
	//Display horizontal divisions
	//
	if (m_bShowGrid)
	{
		for (i = 1; i <= NumHorzDiv; ++i) {
			DivY = YAxisBottomy - i * DivSizePixels;
			egDrawLine(Image, XAxisLeftx,DivY,XAxisRightx, DivY, m_iGridThickness, &GridColour); // Gdiplus::DashStyleDot, m_iGridThickness);
		}
	}

	//
	//Display vertical divisions
	//
	if (m_bShowGrid)
	{
		int		DivX;

		if (!LineChart_IsDiscrete())
		{
			for (i = 1; i <= NumXAxisTextLables; ++i) {
				DivX = i * DivXSizePixels + XAxisLeftx;
				egDrawLine(Image, DivX,YAxisBottomy, DivX, YAxisTopy, m_iGridThickness, &GridColour); // Gdiplus::DashStyleDot, m_iGridThickness);
			}
		}
		else
		{
			for (i = 0; i < MaxNumCat; i += LabelSkip) {
				DivX = (i * CatWidthTotal / MaxNumCat) + CatGap + YAxisTopx;
				if (CatWidthTotal < MaxNumCat)
					DivX += (i * CatGap * CatWidthTotal / MaxNumCat);
				else
					DivX += (i * CatGap);

				egDrawLine(Image, DivX,YAxisBottomy, DivX, YAxisTopy, m_iGridThickness, &GridColour); // Gdiplus::DashStyleDot, m_iGridThickness);
			}
		}
	}

	//
	//Display Y Scale values
	//
	for (i = 0; i < NumHorzDiv; ++i) 
	{
		if(MaxYScale > 10)
		{
			UnicodeSPrint (ValStr, sizeof(ValStr), L"%d", DivSizeUnits * i);
			StrLength = (int)StrLen(ValStr);
			GetTextInfo(ValStr, &TextWidth, &TextHeight);
			DivY = YAxisBottomy - i * DivSizePixels - (int)TextHeight/2;
			ImagePrintXYAlign( Image, ChartLeft, DivY, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, ValStr);
		}
		else
		{
			UnicodeSPrint (ValStr, sizeof(ValStr), L"%ld.%1ld", DivU64x32(MultU64x32(MaxYScale,10),NumHorzDiv), ModU64x32(MultU64x32(MaxYScale,10),NumHorzDiv));
			StrLength = (int)StrLen(ValStr);
			GetTextInfo(ValStr, &TextWidth, &TextHeight);
			DivY = YAxisBottomy - i * DivSizePixels - (int)TextHeight/2;
			ImagePrintXYAlign( Image, ChartLeft, DivY, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, ValStr);
		}
	}

	//
	//Display X Scale values
	//
	if (!LineChart_IsDiscrete()) // For continous x-values only
	{
		for (i = 0; i < NumXAxisTextLables + 1; ++i) 
		{
			int DivX ;
			UnicodeSPrint (ValStr, sizeof(ValStr), L"%ld.%1ld", DivU64x32(MultU64x32(MaxXScale,10),NumXAxisTextLables), ModU64x32(MultU64x32(MaxXScale,10),NumXAxisTextLables));
			StrLength = (int)StrLen(ValStr);
			GetTextInfo(ValStr, &TextWidth, &TextHeight);
			DivX = i * DivXSizePixels + XAxisLeftx - (int)TextHeight/2;
			ImagePrintXYAlign( Image, DivX, ChartTop + VisChartHeight + 5, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, ValStr);
		}
	}

	//
	//Display X and Y axis
	//
	egDrawLine(Image, YAxisTopx, YAxisTopy, YAxisBottomx, YAxisBottomy, AxisWidth, &GridColour);
	egDrawLine(Image, XAxisLeftx, XAxisLefty, XAxisRightx, XAxisRighty, AxisWidth, &GridColour);
	
	//
	//Display lines / series
	//
	YAxisHeight = YAxisBottomy - YAxisTopy;
	XAxisWidth = XAxisRightx - XAxisLeftx;
	LegendLeft = XAxisRightx + LegendGap;
	LegendTop = y + TitleHeight;
	for (iSer = 0; iSer < NumSer; ++iSer) {
		int StartX, StartY;
		//Create a pen with a new colour
		// SColour = RGB (SeriesColours[iSer][0], SeriesColours[iSer][1], SeriesColours[iSer][2]);
		SColour = m_seriesColour[iSer];

		ptx = CatGap + YAxisTopx;

		if (!LineChart_IsDiscrete()) // continous x-values
		{
			{
				int lastptx;
				ptx = XAxisLeftx;
				pty = YAxisBottomy - (int)( DivU64x64Remainder(MultU64x32(ContPoints[iSer][0].y, YAxisHeight), MaxYScale,NULL) );

				StartX = ptx;
				StartY = pty;
		
				lastptx = ptx; //keep track of the last point drawn
				for (i = 0; i < NumContPoints[iSer]; i++) 
				{
					pty =  YAxisBottomy - (int)( DivU64x64Remainder(MultU64x32(ContPoints[iSer][i].y, YAxisHeight), MaxYScale, NULL) );
					ptx =  XAxisLeftx + (int)( DivU64x64Remainder(MultU64x32(ContPoints[iSer][i].x, XAxisWidth), MaxXScale, NULL) );

					if (m_bShowDataPoints[iSer]) // check if data points are to be displayed
					{		
						if ( (ptx - lastptx) >= 5) // only display data points if it is far enough away from the previous point
						{
							int radius = 2;
							egDrawEllipse(Image, ptx, pty, radius, radius, &SColour);
							lastptx = ptx;
						}
					}
				
					// draw line to the point
					egDrawLine(Image, StartX, StartY, ptx, pty, m_iSeriesThickness[iSer], &SColour);
					StartX = ptx;
					StartY = pty;
				}
			}
		}
		else // discrete x-values
		{
			pty =  YAxisBottomy - (int)( DivU64x64Remainder(MultU64x32(CatValues[iSer][0], YAxisHeight), MaxYScale, NULL));

			//if (CatValues[iSer][0] != GRAPH_NOVALUE)	//IR
			StartX = ptx;
			StartY = pty;

			if (m_bShowDataPoints[iSer])
			{
				// int radius = (ChartHeight / 250);
				int radius = 2;
				egDrawEllipse(Image, ptx, pty, radius, radius, &SColour);
			}

			for (i = 1, f = CatWidthTotal, k = 2; i < NumCat[iSer]; f+=MaxNumCat)
			{
				if (f > i * CatWidthTotal) //We've moved at least one data point
				{
					//Average all points up to f that haven't been outputted
					for (j=0, tempCat = 0 ;(i * CatWidthTotal)<f && i < NumCat[iSer]; j++, i++)
						tempCat += CatValues[iSer][i];
					tempCat = DivU64x32(tempCat, j);

					if (m_bShowDataPoints[iSer])
					{
						int radius;
						pty =  YAxisBottomy - (int)( DivU64x64Remainder(MultU64x32(CatValues[iSer][i-1], YAxisHeight), MaxYScale, NULL) );
						ptx = ((i-1) * CatWidthTotal / MaxNumCat) + ((k+1) * CatGap) + YAxisTopx;
						// int radius = (ChartHeight / 250);
						radius = 2;
						egDrawEllipse(Image, ptx, pty, radius, radius, &SColour);
					}

					pty =  YAxisBottomy - (int)(DivU64x64Remainder(MultU64x32(tempCat, YAxisHeight), MaxYScale, NULL));
					ptx = ((i-1) * CatWidthTotal / MaxNumCat) + ((k+1) * CatGap) + YAxisTopx;
					k++;
					
					//if (CatValues[iSer][i-1] == GRAPH_NOVALUE)	//IR
					//	MoveToEx (hdc, ptx, pty, NULL);
					//else if (CatValues[iSer][i] != GRAPH_NOVALUE)	//IR
					egDrawLine(Image, StartX, StartY, ptx, pty, m_iSeriesThickness[iSer], &SColour);


					StartX = ptx;
					StartY = pty;
				}
			}
		}

		//Display the coloured Ledgend boxes
		BoxULx = LegendLeft;
		BoxULy = iSer * (LegendBoxHeight + LegendBoxGap) + LegendTop;
		BoxLRx = BoxULx + LegendBoxWidth;
		BoxLRy = BoxULy + LegendBoxHeight;
		egFillArea (Image, &SColour, BoxULx, BoxULy, LegendBoxWidth, LegendBoxHeight);
	}
	
	//
	//Display Ledgend text
	//
	MaxEnt = 0;
	BoxULx = LegendLeft + LegendBoxWidth + LegendBoxGap/2;
	for (i = 0; i < NumSer; ++i) {
		if (szSerTitles[i])
		{
			if (szSerSubtitles[i])
				BoxULy = i * (LegendBoxHeight + LegendBoxGap) + LegendTop - 1; // -1 to line up with top of the colour box
			else
				BoxULy = i * (LegendBoxHeight + LegendBoxGap) + LegendTop;
			StrLength = (int)StrLen(szSerTitles[i]);
			ImagePrintXYAlign( Image, BoxULx, BoxULy, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, szSerTitles[i]);
			GetTextInfo(szSerTitles[i], &TextWidth, &TextHeight);

			if ((int)TextWidth > MaxEnt)
				MaxEnt = (int)TextWidth;

			if (szSerSubtitles[i])
			{
				ImagePrintXYAlign( Image, BoxULx, BoxULy + TextHeight, BackgroundColour1.r > 128 ? &BlackColour: &WhiteColour, (EFI_GRAPHICS_OUTPUT_BLT_PIXEL *)&BackgroundColour1, ALIGN_LEFT, szSerSubtitles[i]);
			}
		}
	}
	return 0;
}


// Name: SetProperty
// Function: 
//	Sets a particular property for the chart
// Returns: 
// Parameters:
int LineChart_SetProperty(int Property, int NewValue)
{
	if (Property == YAXISUNITS) {
		if (NewValue == YAXISUNITS_MINUTES)
			YAxisUnits = YAXISUNITS_MINUTES;
		else
			YAxisUnits = YAXISUNITS_INTEGER;
	}
	return 0;
}


// Name: SetChartTitle
// Function: 
//	Sets the title for the chart
// Returns: 
// Parameters:
void LineChart_SetChartTitle(CHAR16 * szNewTitle)
{
	//Allocate memory for title if required
	if (szChartTitle == NULL)
		szChartTitle = (CHAR16 *) AllocatePool(TITLE_STRING_LEN);

	StrCpyS (szChartTitle, TITLE_STRING_LEN / sizeof(CHAR16), szNewTitle);

}

CHAR16*	LineChart_GetChartTitle() { 
	return szChartTitle; 
}


// Name: SetXAxisText
// Function: 
//	Sets the text that will appear under the X Axis Values.
//	Usually this would be the Units (eg "Date", "Bytes written")
// Returns: 
// Parameters:
//	IN, NewText,	Pointer to the new text
void LineChart_SetXAxisText(CHAR16 * szNewText)
{
	//Allocate memory for title if required
	if (szXAxisText == NULL)
		szXAxisText = (CHAR16 *) AllocatePool (STRING_LEN);

	StrCpyS (szXAxisText, STRING_LEN / sizeof(CHAR16), szNewText);
}



// Name: GetGraphHeight
// Function: 
//	Calculate the height of the graph
// Parameters:
int LineChart_GetGraphHeight()
{
	int	Height;
	
	Height = (MaxNumCat + 1) * (LegendBoxHeight + LegendBoxGap) + TitleHeight;
	if (Height < ChartWidth + TitleHeight)
		Height = ChartWidth + TitleHeight;
	return (Height);
}


// Name: CalcScale
// Function: 
//	Calculate the Y axis scale
//	and width of columns
// Returns: 
// Parameters:
void CalcScale()
{
	int i, j;
	INT64 MaxYVal = 0 ;
	INT64 MaxXVal = 0;

	MaxXScale = 0;
	MaxYScale = 0;
	MaxNumCat = 0;
	if (!LineChart_IsDiscrete())
	{
		for (j = 0; j < NumSer; ++j) {
			for (i = 0; i < NumContPoints[j]; ++i) {
				if (ContPoints[j][i].y > MaxYVal)
					MaxYVal = (INT64) (ContPoints[j][i].y + 1);	//Add 1.0 to avoid rounding problems
				if (ContPoints[j][i].x > MaxXVal)
					MaxXVal = (INT64) (ContPoints[j][i].x + 1);	//Add 1.0 to avoid rounding problems
			}
		}
	}
	else
	{
		for (j = 0; j < NumSer; ++j) {
			for (i = 0; i < NumCat[j]; ++i) {
				if (CatValues[j][i] > MaxYVal)
					MaxYVal  = (INT64) (CatValues[j][i] + 1);	//Add 1.0 to avoid rounding problems
			}
			if (NumCat[j] > MaxNumCat)
				MaxNumCat = NumCat[j];
		}
	}

	//move the value up to the nearest round number
	if (MaxYVal > 15000) {
		while (MaxYScale < MaxYVal)
			MaxYScale += 10000;
	}
	else
	if (MaxYVal > 1000) {
		while (MaxYScale < MaxYVal)
			MaxYScale += 1000;
	}
	else 
	if (MaxYVal > 100) {
		while (MaxYScale < MaxYVal)
			MaxYScale += 100;
	}
	else
	if (MaxYVal > 10) {
		while (MaxYScale < MaxYVal)
			MaxYScale += 10;
	}
	else
	if (MaxYVal > 1) {
		while (MaxYScale < MaxYVal)
			MaxYScale += 1;
	}
	else {
		MaxYScale = 1;
	}

	//move the value up to the nearest round number
	if (MaxXVal > 60000) {
		while (MaxXScale < MaxXVal)
			MaxXScale += 15000;
	}
	else
	if (MaxXVal > 6000) {
		while (MaxXScale < MaxXVal)
			MaxXScale += 1500;
	}
	else 
	if (MaxXVal > 600) {
		while (MaxXScale < MaxXVal)
			MaxXScale += 150;
	}
	else
	if (MaxXVal > 60) {
		while (MaxXScale < MaxXVal)
			MaxXScale += 15;
	}
	else
	if (MaxXVal > 3) {
		while (MaxXScale < MaxXVal)
			MaxXScale += 3;
	}
	else {
		MaxXScale = 3;
	}
}