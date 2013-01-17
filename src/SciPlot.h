/*-----------------------------------------------------------------------------
** SciPlot.h	A generalized plotting widget
**
** Public header file
**
** Copyright (c) 1994 Robert W. McMullen
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

#ifndef _SCIPLOT_H
#define _SCIPLOT_H

#include <X11/Core.h>

#define _SCIPLOT_WIDGET_VERSION	1.11

#ifndef XtIsSciPlot
#define XtIsSciPlot(w) XtIsSubclass((Widget)w, sciplotWidgetClass)
#endif 


typedef float real;

typedef struct {
	real x,y;
} realpair;


#define XtNchartType	"chartType"
#define XtNdegrees	"degrees"
#define XtNdrawMajor	"drawMajor"
#define XtNdrawMajorTics	"drawMajorTics"
#define XtNdrawMinor	"drawMinor"
#define XtNdrawMinorTics	"drawMinorTics"
#define XtNxAutoScale	"xAutoScale"
#define XtNyAutoScale	"yAutoScale"
#define XtNxLog		"xLog"
#define XtNyLog		"yLog"
#define XtNxOrigin	"xOrigin"
#define XtNyOrigin	"yOrigin"
#define XtNxLabel	"xLabel"
#define XtNyLabel	"yLabel"
#define XtNplotTitle	"plotTitle"
#define XtNmargin	"margin"
#define XtNtitleMargin	"titleMargin"
#define XtNshowLegend	"showLegend"
#define XtNshowTitle	"showTitle"
#define XtNshowXLabel	"showXLabel"
#define XtNshowYLabel	"showYLabel"
#define XtNlegendLineSize	"legendLineSize"
#define XtNlegendMargin	"legendMargin"
#define XtNtitleFont	"titleFont"
#define XtNlabelFont	"labelFont"
#define XtNaxisFont	"axisFont"

#define XtPOLAR		0
#define XtCARTESIAN	1

#define XtMARKER_NONE		0
#define XtMARKER_CIRCLE		1
#define XtMARKER_SQUARE		2
#define XtMARKER_UTRIANGLE	3
#define XtMARKER_DTRIANGLE	4
#define XtMARKER_LTRIANGLE	5
#define XtMARKER_RTRIANGLE	6
#define XtMARKER_DIAMOND	7
#define XtMARKER_HOURGLASS	8
#define XtMARKER_BOWTIE		9
#define XtMARKER_FCIRCLE	10
#define XtMARKER_FSQUARE	11
#define XtMARKER_FUTRIANGLE	12
#define XtMARKER_FDTRIANGLE	13
#define XtMARKER_FLTRIANGLE	14
#define XtMARKER_FRTRIANGLE	15
#define XtMARKER_FDIAMOND	16
#define XtMARKER_FHOURGLASS	17
#define XtMARKER_FBOWTIE	18

#define XtFONT_SIZE_MASK	0xff
#define XtFONT_SIZE_DEFAULT	12

#define XtFONT_NAME_MASK	0xf00
#define XtFONT_TIMES		0x000
#define XtFONT_COURIER		0x100
#define XtFONT_HELVETICA	0x200
#define XtFONT_LUCIDA		0x300
#define XtFONT_LUCIDASANS	0x400
#define XtFONT_NCSCHOOLBOOK	0x500
#define XtFONT_NAME_DEFAULT	XtFONT_TIMES

#define XtFONT_ATTRIBUTE_MASK	0xf000
#define XtFONT_BOLD		0x1000
#define XtFONT_ITALIC		0x2000
#define XtFONT_BOLD_ITALIC	0x3000
#define XtFONT_ATTRIBUTE_DEFAULT 0


#define XtLINE_NONE	0
#define XtLINE_SOLID	1
#define XtLINE_DOTTED	2
#define XtLINE_WIDEDOT	3
#define XtLINE_USERDASH	4

extern WidgetClass sciplotWidgetClass;

typedef struct _SciPlotClassRec *SciPlotWidgetClass;
typedef struct _SciPlotRec      *SciPlotWidget;


/*
** Public function declarations
*/

#if __STDC__ || defined(__cplusplus)
#define P_(s) s
#else
#define P_(s) ()
#endif

/* SciPlot.c */
Boolean SciPlotPSCreateFancy P_((SciPlotWidget w, char *filename, int drawborder, char *titles));
Boolean SciPlotPSCreate P_((Widget wi, char *filename));
int SciPlotAllocNamedColor P_((Widget wi, char *name));
int SciPlotAllocRGBColor P_((Widget wi, int r, int g, int b));
void SciPlotSetBackgroundColor P_((Widget wi, int color));
void SciPlotSetForegroundColor P_((Widget wi, int color));
int SciPlotListCreateFromData P_((Widget wi, int num, real *xlist, real *ylist, char *legend, int pcolor, int pstyle, int lcolor, int lstyle));
int SciPlotListCreateFromFloat P_((Widget wi, int num, float *xlist, float *ylist, char *legend));
void SciPlotListDelete P_((Widget wi, int id));
void SciPlotListUpdateFromFloat P_((Widget wi, int id, int num, float *xlist, float *ylist));
int SciPlotListCreateFromDouble P_((Widget wi, int num, double *xlist, double *ylist, char *legend));
void SciPlotListUpdateFromDouble P_((Widget wi, int id, int num, double *xlist, double *ylist));
void SciPlotListSetStyle P_((Widget wi, int id, int pcolor, int pstyle, int lcolor, int lstyle));
void SciPlotSetXAutoScale P_((Widget wi));
void SciPlotSetXUserScale P_((Widget wi, double min, double max));
void SciPlotSetYAutoScale P_((Widget wi));
void SciPlotSetYUserScale P_((Widget wi, double min, double max));
void SciPlotPrintStatistics P_((Widget wi));
void SciPlotExportData P_((Widget wi, FILE *fd));
void SciPlotUpdate P_((Widget wi));
void SciPlotQueryXScale P_((Widget wi, float *min, float *max));
void SciPlotQueryYScale P_((Widget wi, float *min, float *max));
void SciPlotQueryXAxisValues( Widget w, int *nvals, float **values);
void SciPlotSetXAxisLabels(Widget wi,int nlabels, char **labels);
float SciPlotScreenToDataX(Widget wi,int xscreen);
float SciPlotScreenToDataY(Widget wi,int yscreen);
void  SciPlotAddXAxisCallback(Widget wi, void (*cb)(Widget,float,char*,size_t) );

#undef P_

#endif /* _SCIPLOT_H */
