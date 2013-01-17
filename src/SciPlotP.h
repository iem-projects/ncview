/*-----------------------------------------------------------------------------
** SciPlotP.h	A generalized plotting widget
**
** Private header file
**
** Copyright (c) 1995 Robert W. McMullen
**
** Permission to use, copy, modify, distribute, and sell this software and its
** documentation for any purpose is hereby granted without fee, provided that
** the above copyright notice appear in all copies and that both that
** copyright notice and this permission notice appear in supporting
** documentation.  The author makes no representations about the suitability
** of this software for any purpose.  It is provided "as is" without express
** or implied warranty.
**
** THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
** ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL
** THE AUTHOR BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
** ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
** WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
** ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
** SOFTWARE.
*/

#ifndef _SCIPLOTP_H
#define _SCIPLOTP_H

#include <X11/CoreP.h>
#ifdef MOTIF
#if XmVersion == 1001
#include <Xm/XmP.h>
#else
#include <Xm/PrimitiveP.h>
#endif
#endif
#include <math.h>
#ifdef  __sgi
#include <sys/types.h>
#include <malloc.h>
#endif  /* __sgi */

#include "SciPlot.h"
#define powi(a,i)	(real)pow(a,(double)((int)i))

#define NUMPLOTLINEALLOC	5
#define NUMPLOTDATAEXTRA	25
#define NUMPLOTITEMALLOC	256
#define NUMPLOTITEMEXTRA	64
#define DEG2RAD       (3.1415926535897931160E0/180.0)

typedef struct {
	int	dummy;	/* keep compiler happy with dummy field */
} SciPlotClassPart;


typedef struct _SciPlotClassRec {
	CoreClassPart		core_class;
#ifdef MOTIF
	XmPrimitiveClassPart	primitive_class;
#endif
	SciPlotClassPart	plot_class;
} SciPlotClassRec;

extern SciPlotClassRec sciplotClassRec;

typedef enum {
	SciPlotFALSE,
	SciPlotPoint,
	SciPlotLine,
	SciPlotRect,
	SciPlotFRect,
	SciPlotCircle,
	SciPlotFCircle,
	SciPlotStartTextTypes,
	SciPlotText,
	SciPlotVText,
	SciPlotEndTextTypes,
	SciPlotPoly,
	SciPlotFPoly,
	SciPlotClipRegion,
	SciPlotClipClear,
	SciPlotENDOFLIST
} SciPlotTypeEnum;

typedef struct _SciPlotItem {
	SciPlotTypeEnum	type;
	union {
		struct {
			short color;
			short style;
			real x,y;
		} pt;
		struct {
			short color;
			short style;
			real x1,y1,x2,y2;
		} line;
		struct {
			short color;
			short style;
			real x,y,w,h;
		} rect;
		struct {
			short color;
			short style;
			real x,y,r;
		} circ;
		struct {
			short color;
			short style;
			short count;
			real x[4],y[4];
		} poly;
		struct {
			short color;
			short style;
			short font;
			short length;
			real x,y;
			char *text;
		} text;
		struct {
			short color;
			short style;
		} any;
			
	} kind;
	short individually_allocated;
	struct _SciPlotItem *next;
} SciPlotItem;

typedef struct {
	int		LineStyle;
	int		LineColor;
	int		PointStyle;
	int		PointColor;
	int		number;
	int		allocated;
	realpair	*data;
	char		*legend;
	realpair	min,max;
	Boolean		draw,used;
} SciPlotList;

typedef struct {
	real		Origin;
	real		Size;
	real		Center;
	real		TitlePos;
	real		AxisPos;
	real		LabelPos;
	real		LegendPos;
	real		LegendSize;
	real		DrawOrigin;
	real		DrawSize;
	real		DrawMax;
	real		MajorInc;
	int		MajorNum;
	int		MinorNum;
	int		Precision;
	double		Scalefact;
	int		Scale_expon;
} SciPlotAxis;

typedef struct {
	int		id;
	XFontStruct	*font;
} SciPlotFont;

typedef struct {
	int		flag;
	char		*PostScript;
	char		*X11;
	Boolean		PSUsesOblique;
	Boolean		PSUsesRoman;
} SciPlotFontDesc;

typedef struct {
	/* Public stuff ... */
	char		*TransientPlotTitle;
	char		*TransientXLabel;
	char		*TransientYLabel;
	int		Margin;
	int		TitleMargin;
	int		LegendMargin;
	int		LegendLineSize;
	int		MajorTicSize;
	int		ChartType;
	Boolean		ScaleToFit;
	Boolean		Degrees;
	Boolean		XLog;
	Boolean		YLog;
	Boolean		XAutoScale;
	Boolean		YAutoScale;
	Boolean		XOrigin;
	Boolean		YOrigin;
	Boolean		DrawMajor;
	Boolean		DrawMinor;
	Boolean		DrawMajorTics;
	Boolean		DrawMinorTics;
	Boolean		ShowLegend;
	Boolean		ShowTitle;
	Boolean		ShowXLabel;
	Boolean		ShowYLabel;
	Font		TitleFID;
	int		TitleFont;
	int		LabelFont;
	int		AxisFont;
	int		BackgroundColor;
	int		ForegroundColor;

	/* Private stuff ... */
	char		*plotTitle;
	char		*xlabel;
	char		*ylabel;
	realpair	Min,Max;
	realpair	UserMin,UserMax;
	real		PolarScale;
	SciPlotAxis	x,y;
	int		titleFont;
	int		labelFont;
	int		axisFont;
	void		(*XFmtCallback)(Widget w, float f, char *s, size_t slen);

	GC		defaultGC;
	GC		dashGC;
	Colormap	cmap;
	Pixel		*colors;
	int		num_colors;
	SciPlotFont	*fonts;
	int		num_fonts;

	int		alloc_plotlist;
	int		num_plotlist;
	SciPlotList	*plotlist;
	int		alloc_drawlist;
	int		num_drawlist;
	SciPlotItem	*drawlist;
	Boolean		update;
} SciPlotPart;

typedef struct _SciPlotRec {
	CorePart	core;
#ifdef MOTIF
	XmPrimitivePart	primitive;
#endif
	SciPlotPart	plot;
} SciPlotRec;


#endif /* _SCIPLOTP_H */
