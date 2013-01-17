/*	define DEBUG_SCIPLOT */
/*-----------------------------------------------------------------------------
** SciPlot.c	A generalized plotting widget
**
** Widget source code
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

#include <math.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <stdio.h>
#include <stdlib.h>

#include "SciPlotP.h"

#define offset(field) XtOffsetOf(SciPlotRec, plot.field)
static XtResource resources[] = {
	{XtNchartType, XtCMargin, XtRInt, sizeof(int),
		offset(ChartType), XtRImmediate, (XtPointer)XtCARTESIAN},
	{XtNdegrees, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(Degrees), XtRString, "TRUE"},
	{XtNdrawMajor, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMajor), XtRString, "TRUE"},
	{XtNdrawMajorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMajorTics), XtRString, "TRUE"},
	{XtNdrawMinor, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMinor), XtRString, "TRUE"},
	{XtNdrawMinorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(DrawMinorTics), XtRString, "TRUE"},
	{XtNshowLegend, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowLegend), XtRString, "TRUE"},
	{XtNshowTitle, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowTitle), XtRString, "TRUE"},
	{XtNshowXLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowXLabel), XtRString, "TRUE"},
	{XtNshowYLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(ShowYLabel), XtRString, "TRUE"},
	{XtNxLabel, XtCString, XtRString, sizeof(String),
		offset(TransientXLabel), XtRString, "X Axis"},
	{XtNyLabel, XtCString, XtRString, sizeof(String),
		offset(TransientYLabel), XtRString, "Y Axis"},
	{XtNplotTitle, XtCString, XtRString, sizeof(String),
		offset(TransientPlotTitle), XtRString, "Plot"},
	{XtNmargin, XtCMargin, XtRInt, sizeof(int),
		offset(Margin), XtRString, "5"},
	{XtNtitleMargin, XtCMargin, XtRInt, sizeof(int),
		offset(TitleMargin), XtRString, "16"},
	{XtNlegendLineSize, XtCMargin, XtRInt, sizeof(int),
		offset(LegendLineSize), XtRString, "16"},
	{XtNlegendMargin, XtCMargin, XtRInt, sizeof(int),
		offset(LegendMargin), XtRString, "3"},
	{XtNtitleFont, XtCMargin, XtRInt, sizeof(int),
		offset(TitleFont), XtRImmediate,
		(XtPointer)(XtFONT_HELVETICA|24)},
	{XtNlabelFont, XtCMargin, XtRInt, sizeof(int),
		offset(LabelFont), XtRImmediate,
		(XtPointer)(XtFONT_TIMES|18)},
	{XtNaxisFont, XtCMargin, XtRInt, sizeof(int),
		offset(AxisFont), XtRImmediate,
		(XtPointer)(XtFONT_TIMES|10)},
	{XtNxAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XAutoScale), XtRString, "TRUE"},
	{XtNyAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YAutoScale), XtRString, "TRUE"},
	{XtNxLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XLog), XtRString, "FALSE"},
	{XtNyLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YLog), XtRString, "FALSE"},
	{XtNxOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(XOrigin), XtRString, "FALSE"},
	{XtNyOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
		offset(YOrigin), XtRString, "FALSE"},
};

static SciPlotFontDesc font_desc_table[]={
	{XtFONT_TIMES,		"Times",	"times",	False,True},
	{XtFONT_COURIER,	"Courier",	"courier",	True,False},
	{XtFONT_HELVETICA,	"Helvetica",	"helvetica",	True,False},
	{XtFONT_LUCIDA,		"Lucida",	"lucidabright",	False,False},
	{XtFONT_LUCIDASANS,	"LucidaSans",	"lucida",	False,False},
	{XtFONT_NCSCHOOLBOOK,	"NewCenturySchlbk",
					"new century schoolbook",False,True},
	{-1,NULL,NULL,False,False},
};

/*
** Private function declarations
*/

static void Redisplay();
static void Resize();
static Boolean SetValues();
static void Initialize();
static void Destroy();

static void ComputeAll();
static void ComputeAllDimensions();
static void DrawAll();
static void ItemDrawAll();
static void ItemDraw();
static void EraseAll();
static void FontInit();
static int ColorStore();
static int FontStore();
static int FontnumReplace();



SciPlotClassRec sciplotClassRec = {
    {
    /* core_class fields	*/
#ifdef MOTIF
    /* superclass		*/ (WidgetClass) &xmPrimitiveClassRec,
#else
    /* superclass		*/ (WidgetClass) &widgetClassRec,
#endif
    /* class_name		*/ "SciPlot",
    /* widget_size		*/ sizeof(SciPlotRec),
    /* class_initialize		*/ NULL,
    /* class_part_initialize	*/ NULL,
    /* class_inited		*/ False,
    /* initialize		*/ Initialize,
    /* initialize_hook		*/ NULL,
    /* realize			*/ XtInheritRealize,
    /* actions			*/ NULL,
    /* num_actions		*/ 0,
    /* resources		*/ resources,
    /* num_resources		*/ XtNumber(resources),
    /* xrm_class		*/ NULLQUARK,
    /* compress_motion		*/ True,
    /* compress_exposure	*/ XtExposeCompressMultiple,
    /* compress_enterleave	*/ True,
    /* visible_interest		*/ True,
    /* destroy			*/ Destroy,
    /* resize			*/ Resize,
    /* expose			*/ Redisplay,
    /* set_values		*/ SetValues,
    /* set_values_hook		*/ NULL,
    /* set_values_almost	*/ XtInheritSetValuesAlmost,
    /* get_values_hook		*/ NULL,
    /* accept_focus		*/ NULL,
    /* version			*/ XtVersion,
    /* callback_private		*/ NULL,
    /* tm_table			*/ NULL,
    /* query_geometry		*/ NULL,
    /* display_accelerator	*/ XtInheritDisplayAccelerator,
    /* extension		*/ NULL
    },
#ifdef MOTIF
    {
    /* primitive_class fields	*/
    /* border_highlight		*/ (XtWidgetProc)_XtInherit,
    /* border_unhighligh	*/ (XtWidgetProc)_XtInherit,
    /* translations		*/ XtInheritTranslations,
    /* arm_and_activate		*/ (XtWidgetProc)_XtInherit,
    /* syn_resources		*/ NULL,
    /* num_syn_resources	*/ 0,
    /* extension		*/ NULL
    },
#endif
    {
    /* plot_class fields	*/
    /* dummy			*/ 0
    /* (some stupid compilers barf on empty structures) */
    }
};

WidgetClass sciplotWidgetClass = (WidgetClass)&sciplotClassRec;

void
get_number_format( char *numberformat, size_t nflen, int precision, real val )
{
	/*sprintf(numberformat,"%%.%df",precision);*/
	snprintf(numberformat,nflen,"%%%dg",precision);
}

int
my_nint( double d )
{
	if( d >= 0.0 ) 
		return( (int)(d+0.4999) );

	return( (int)(d-0.4999) );
}

static void
Initialize(treq,tnew,args,num)
Widget treq,tnew;
ArgList args;
Cardinal *num;
{
SciPlotWidget new;
XGCValues values;
XtGCMask mask;
long	colorsave;

	new=(SciPlotWidget) tnew;
	
	new->plot.plotlist=NULL;
	new->plot.alloc_plotlist=0;
	new->plot.num_plotlist=0;
	
	new->plot.alloc_drawlist=NUMPLOTITEMALLOC;
	new->plot.drawlist=(SciPlotItem *)XtCalloc(new->plot.alloc_drawlist,
		sizeof(SciPlotItem));
	new->plot.num_drawlist=0;

	new->plot.cmap = DefaultColormap(XtDisplay(new),
		DefaultScreen(XtDisplay(new)));
	
	new->plot.xlabel=(char *)XtMalloc(strlen(new->plot.TransientXLabel)+1);
	strcpy(new->plot.xlabel,new->plot.TransientXLabel);
	new->plot.ylabel=(char *)XtMalloc(strlen(new->plot.TransientYLabel)+1);
	strcpy(new->plot.ylabel,new->plot.TransientYLabel);
	new->plot.plotTitle=(char *)XtMalloc(strlen(new->plot.TransientPlotTitle)+1);
	strcpy(new->plot.plotTitle,new->plot.TransientPlotTitle);

	new->plot.XFmtCallback=NULL;
	new->plot.colors=NULL;
	new->plot.num_colors=0;
	new->plot.fonts=NULL;
	new->plot.num_fonts=0;
	
	new->plot.update=FALSE;
	new->plot.UserMin.x=new->plot.UserMin.y=0.0;
	new->plot.UserMax.x=new->plot.UserMax.y=10.0;

	values.line_style=LineSolid;
	values.line_width=0;
	values.fill_style=FillSolid;
	values.background=WhitePixelOfScreen(XtScreen(new));
	new->plot.BackgroundColor=ColorStore(new,values.background);
#ifdef MOTIF
	new->core.background_pixel=values.background;
#endif
	values.foreground=colorsave=BlackPixelOfScreen(XtScreen(new));
	new->plot.ForegroundColor=ColorStore(new,values.foreground);

	mask=GCLineStyle|GCLineWidth|GCFillStyle|GCForeground|GCBackground;
	new->plot.defaultGC=XtGetGC((Widget)new,mask,&values);

	values.foreground=colorsave;
	values.line_style=LineOnOffDash;
	new->plot.dashGC=XtGetGC((Widget)new,mask,&values);
	
	new->plot.titleFont=FontStore(new,new->plot.TitleFont);
	new->plot.labelFont=FontStore(new,new->plot.LabelFont);
	new->plot.axisFont=FontStore(new,new->plot.AxisFont);
}

static void
Destroy(w)
SciPlotWidget w;
{
int i;
SciPlotFont *pf;
SciPlotList *p;

	XtReleaseGC((Widget)w,w->plot.defaultGC);
	XtReleaseGC((Widget)w,w->plot.dashGC);
	XtFree((char *)w->plot.xlabel);
	XtFree((char *)w->plot.ylabel);
	XtFree((char *)w->plot.plotTitle);

	for (i=0; i<w->plot.num_fonts; i++) {
		pf=&w->plot.fonts[i];
		XFreeFont(XtDisplay((Widget)w),pf->font);
	}
	XtFree((char *)w->plot.fonts);

	XtFree((char *)w->plot.colors);

	for (i=0; i<w->plot.alloc_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->allocated>0) XtFree((char *)p->data);
		if (p->legend) XtFree(p->legend);
	}
	if (w->plot.alloc_plotlist>0)
		XtFree((char *)w->plot.plotlist);
		
	EraseAll(w);
	XtFree((char *)w->plot.drawlist);
}

static Boolean
SetValues(current, request, new, args, nargs)
SciPlotWidget current, request, new;
ArgList args;
Cardinal *nargs;
{
Boolean redisplay=FALSE;

	if (current->plot.XLog!=new->plot.XLog) redisplay=TRUE;
	else if (current->plot.YLog!=new->plot.YLog) redisplay=TRUE;
	else if (current->plot.XOrigin!=new->plot.XOrigin) redisplay=TRUE;
	else if (current->plot.YOrigin!=new->plot.YOrigin) redisplay=TRUE;
	else if (current->plot.DrawMajor!=new->plot.DrawMajor) redisplay=TRUE;
	else if (current->plot.DrawMajorTics!=new->plot.DrawMajorTics) redisplay=TRUE;
	else if (current->plot.DrawMinor!=new->plot.DrawMinor) redisplay=TRUE;
	else if (current->plot.DrawMinorTics!=new->plot.DrawMinorTics) redisplay=TRUE;
	else if (current->plot.ChartType!=new->plot.ChartType) redisplay=TRUE;
	else if (current->plot.Degrees!=new->plot.Degrees) redisplay=TRUE;
	else if (current->plot.ShowLegend!=new->plot.ShowLegend) redisplay=TRUE;
	else if (current->plot.ShowTitle!=new->plot.ShowTitle) redisplay=TRUE;
	else if (current->plot.ShowXLabel!=new->plot.ShowXLabel) redisplay=TRUE;
	else if (current->plot.ShowYLabel!=new->plot.ShowYLabel) redisplay=TRUE;
	else if (current->plot.ShowTitle!=new->plot.ShowTitle) redisplay=TRUE;
	if (current->plot.TransientXLabel!=new->plot.TransientXLabel) {
		redisplay=TRUE;
		XtFree(current->plot.xlabel);
		new->plot.xlabel=(char *)XtMalloc(strlen(new->plot.TransientXLabel)+1);
		strcpy(new->plot.xlabel,new->plot.TransientXLabel);
	}
	if (current->plot.TransientYLabel!=new->plot.TransientYLabel) {
		redisplay=TRUE;
		XtFree(current->plot.ylabel);
		new->plot.ylabel=(char *)XtMalloc(strlen(new->plot.TransientYLabel)+1);
		strcpy(new->plot.ylabel,new->plot.TransientYLabel);
	}
	if (current->plot.TransientPlotTitle!=new->plot.TransientPlotTitle) {
		redisplay=TRUE;
		XtFree(current->plot.plotTitle);
		new->plot.plotTitle=(char *)XtMalloc(strlen(new->plot.TransientPlotTitle)+1);
		strcpy(new->plot.plotTitle,new->plot.TransientPlotTitle);
	}
	if (current->plot.AxisFont!=new->plot.AxisFont) {
		redisplay=TRUE;
		FontnumReplace(new,new->plot.axisFont,new->plot.AxisFont);
	}
	if (current->plot.TitleFont!=new->plot.TitleFont) {
		redisplay=TRUE;
		FontnumReplace(new,new->plot.titleFont,new->plot.TitleFont);
	}
	if (current->plot.LabelFont!=new->plot.LabelFont) {
		redisplay=TRUE;
		FontnumReplace(new,new->plot.labelFont,new->plot.LabelFont);
	}

	new->plot.update=redisplay;
	
	return redisplay;
}

static void Redisplay(w)
SciPlotWidget w;
{
	if (w->plot.update) {
		Resize(w);
		w->plot.update=FALSE;
	}
	else {
		ItemDrawAll(w);
	}
}

static void Resize(w)
SciPlotWidget w;
{
	EraseAll(w);
	ComputeAll(w);
	DrawAll(w);
}


/*
** Private SciPlot utility functions --------------------------------------------
**
*/



static int
ColorStore(w,color)
SciPlotWidget w;
Pixel color;
{
	w->plot.num_colors++;
	if (w->plot.colors)
		w->plot.colors=(Pixel *)XtRealloc((char *)w->plot.colors,
			sizeof(Pixel)*w->plot.num_colors);
	else
		w->plot.colors=(Pixel *)XtCalloc(1,sizeof(Pixel));
	w->plot.colors[w->plot.num_colors-1]=color;
	return w->plot.num_colors-1;
}

static void
FontnumStore(w,fontnum,flag)
SciPlotWidget w;
int fontnum,flag;
{
SciPlotFont *pf;
int fontflag,sizeflag,attrflag;

	pf=&w->plot.fonts[fontnum];
	
	fontflag=flag&XtFONT_NAME_MASK;
	sizeflag=flag&XtFONT_SIZE_MASK;
	attrflag=flag&XtFONT_ATTRIBUTE_MASK;
	
	switch (fontflag) {
	case XtFONT_TIMES:
	case XtFONT_COURIER:
	case XtFONT_HELVETICA:
	case XtFONT_LUCIDA:
	case XtFONT_LUCIDASANS:
	case XtFONT_NCSCHOOLBOOK:
		break;
	default:
		fontflag=XtFONT_NAME_DEFAULT;
		break;
	}
	
	if (sizeflag<1) sizeflag=XtFONT_SIZE_DEFAULT;
	
	switch (attrflag) {
	case XtFONT_BOLD:
	case XtFONT_ITALIC:
	case XtFONT_BOLD_ITALIC:
		break;
	default:
		attrflag=XtFONT_ATTRIBUTE_DEFAULT;
		break;
	}
	pf->id=flag;
	FontInit(w,pf);
}

static int
FontnumReplace(w,fontnum,flag)
SciPlotWidget w;
int fontnum,flag;
{
SciPlotFont *pf;

	pf=&w->plot.fonts[fontnum];
	XFreeFont(XtDisplay(w),pf->font);
	
	FontnumStore(w,fontnum,flag);
	
	return fontnum;
}

static int
FontStore(w,flag)
SciPlotWidget w;
int flag;
{
int fontnum;

	w->plot.num_fonts++;
	if (w->plot.fonts)
		w->plot.fonts=(SciPlotFont *)XtRealloc((char *)w->plot.fonts,
			sizeof(SciPlotFont)*w->plot.num_fonts);
	else
		w->plot.fonts=(SciPlotFont *)XtCalloc(1,sizeof(SciPlotFont));
	fontnum=w->plot.num_fonts-1;
	
	FontnumStore(w,fontnum,flag);
	
	return fontnum;
}

static SciPlotFontDesc *
FontDescLookup(flag)
int flag;
{
SciPlotFontDesc *pfd;
	
	pfd=font_desc_table;
	while (pfd->flag>=0) {
#ifdef DEBUG_SCIPLOT
		printf("checking if %d == %d (font %s)\n",
			flag&XtFONT_NAME_MASK,pfd->flag,pfd->PostScript);
#endif
		if ((flag&XtFONT_NAME_MASK)==pfd->flag) return pfd;
		pfd++;
	}
	return NULL;
}


static void
FontnumPostScriptString(w,fontnum,str,str_len)
SciPlotWidget w;
int fontnum;
char *str;
int str_len;
{
char temp[128];
int flag,bold,italic;
SciPlotFontDesc *pfd;

	flag=w->plot.fonts[fontnum].id;
	pfd=FontDescLookup(flag);
	if (pfd) {
		strcpy(temp,pfd->PostScript);
		bold=False;
		italic=False;
		if (flag&XtFONT_BOLD) {
			bold=True;
			strcat(temp,"-Bold");
		}
		if (flag&XtFONT_ITALIC) {
			italic=True;
			if (!bold) strcat(temp,"-");
			if (pfd->PSUsesOblique) strcat(temp,"Oblique");
			else strcat(temp,"Italic");
		}
		if (!bold && !italic && pfd->PSUsesRoman) {
			strcat(temp,"-Roman");
		}

		if( (strlen(temp) + strlen("/ findfont 999999999 scalefont    ")) > str_len ) {
			fprintf( stderr, "Error, font name too long for internal buffer.  Font name:%s\n",
				temp );
			exit(-1);
			}
		snprintf(str, str_len, "/%s findfont %d scalefont",
			temp,
			(flag&XtFONT_SIZE_MASK));
	}
	else snprintf(str,str_len,"/Courier findfond 10 scalefont");
}

static void
FontX11String(flag,str,slen)
int flag;
char *str;
int slen;
{
SciPlotFontDesc *pfd;

	pfd=FontDescLookup(flag);
	if (pfd) {
		snprintf(str,slen,"-*-%s-%s-%s-*-*-%d-*-*-*-*-*-*-*",
			pfd->X11,
			(flag&XtFONT_BOLD?"bold":"medium"),
			(flag&XtFONT_ITALIC?(pfd->PSUsesOblique?"o":"i"):"r"),
			(flag&XtFONT_SIZE_MASK));
	}
	else snprintf(str,slen,"fixed");
#ifdef DEBUG_SCIPLOT
	printf("font string=%s\n",str);
#endif
}

static void
FontInit(w,pf)
SciPlotWidget w;
SciPlotFont *pf;
{
char str[256],**list;
int num;

	FontX11String(pf->id,str,256);
	list=XListFonts(XtDisplay(w),str,100,&num);
#ifdef DEBUG_SCIPLOT
	if (1) {
	int i;
		i=0;
		while (i<num) {
			printf("Found font: %s\n",list[i]);
			i++;
		}
	}
#endif
	if (num<=0) {
		pf->id&=~XtFONT_ATTRIBUTE_MASK;
		pf->id|=XtFONT_ATTRIBUTE_DEFAULT;
		FontX11String(pf->id,str,256);
		list=XListFonts(XtDisplay(w),str,100,&num);
#ifdef DEBUG_SCIPLOT
	if (1) {
	int i;
		i=0;
		while (i<num) {
			printf("Attr reset: found: %s\n",list[i]);
			i++;
		}
	}
#endif
	}
	if (num<=0) {
		pf->id&=~XtFONT_NAME_MASK;
		pf->id|=XtFONT_NAME_DEFAULT;
		FontX11String(pf->id,str,256);
		list=XListFonts(XtDisplay(w),str,100,&num);
#ifdef DEBUG_SCIPLOT
	if (1) {
	int i;
		i=0;
		while (i<num) {
			printf("Name reset: found: %s\n",list[i]);
			i++;
		}
	}
#endif
	}
	if (num<=0) {
		pf->id&=~XtFONT_SIZE_MASK;
		pf->id|=XtFONT_SIZE_DEFAULT;
		FontX11String(pf->id,str,256);
		list=XListFonts(XtDisplay(w),str,100,&num);
#ifdef DEBUG_SCIPLOT
	if (1) {
	int i;
		i=0;
		while (i<num) {
			printf("Size reset: found: %s\n",list[i]);
			i++;
		}
	}
#endif
	}
	if (num<=0)
		strcpy(str,"fixed");
	else
		XFreeFontNames(list);
	pf->font=XLoadQueryFont(XtDisplay(w),str);
}

static XFontStruct
*FontFromFontnum(w,fontnum)
SciPlotWidget w;
int fontnum;
{
XFontStruct *f;

	if (fontnum>=w->plot.num_fonts) fontnum=0;
	f=w->plot.fonts[fontnum].font;
	return f;
}

static real
FontHeight(f)
XFontStruct *f;
{
	return (real)(f->max_bounds.ascent+f->max_bounds.descent);
}

static real
FontnumHeight(w,fontnum)
SciPlotWidget w;
int fontnum;
{
XFontStruct *f;

	f=FontFromFontnum(w,fontnum);
	return FontHeight(f);
}

static real
FontDescent(f)
XFontStruct *f;
{
	return (real)(f->max_bounds.descent);
}

static real
FontnumDescent(w,fontnum)
SciPlotWidget w;
int fontnum;
{
XFontStruct *f;

	f=FontFromFontnum(w,fontnum);
	return FontDescent(f);
}

static real
FontAscent(f)
XFontStruct *f;
{
	return (real)(f->max_bounds.ascent);
}

static real
FontnumAscent(w,fontnum)
SciPlotWidget w;
int fontnum;
{
XFontStruct *f;

	f=FontFromFontnum(w,fontnum);
	return FontAscent(f);
}

static real
FontTextWidth(f,c)
XFontStruct *f;
char *c;
{
	return (real)XTextWidth(f,c,strlen(c));
}

static real
FontnumTextWidth(w,fontnum,c)
SciPlotWidget w;
int fontnum;
char *c;
{
XFontStruct *f;

	f=FontFromFontnum(w,fontnum);
	return FontTextWidth(f,c);
}

static void
ItemDrawAll(w)
SciPlotWidget w;
{
SciPlotItem	*item;
int	i;

	if (!XtIsRealized((Widget)w)) return;
	item=w->plot.drawlist;
	i=0;
	while (i<w->plot.num_drawlist) {
		ItemDraw(w,item);
		i++;
		item++;
	}
}


/*
** Private SciPlot functions -------------------------------------------------
**
*/


/* The following vertical text drawing routine uses the "Fill Stippled" idea
** found in xvertext-5.0, by Alan Richardson (mppa3@syma.sussex.ac.uk).
**
** The following code is my interpretation of his idea, including some
** hacked together excerpts from his source.  The credit for the clever bits
** belongs to him.
**
** To be complete, portions of the subroutine XDrawVString are
**  Copyright (c) 1993 Alan Richardson (mppa3@syma.sussex.ac.uk)
*/
static void
XDrawVString(display,win,gc,x,y,str,len,f)
Display *display;
Window win;
GC gc;
int x,y;
char *str;
int len;
XFontStruct *f;
{
XImage *before,*after;
char *dest,*source;
int xloop,yloop,xdest,ydest;
Pixmap pix,rotpix;
int width,height;
GC drawGC;

	width=(int)FontTextWidth(f,str);
	height=(int)FontHeight(f);

	pix=XCreatePixmap(display,win,width,height,1);
	rotpix=XCreatePixmap(display,win,height,width,1);

	drawGC=XCreateGC(display,pix,0L,NULL);
	XSetBackground(display,drawGC,0);
	XSetFont(display,drawGC,f->fid);
	XSetForeground(display,drawGC,0);
	XFillRectangle(display,pix,drawGC,0,0,width,height);
	XFillRectangle(display,rotpix,drawGC,0,0,height,width);
	XSetForeground(display,drawGC,1);

	XDrawImageString(display,pix,drawGC,0,(int)FontAscent(f),
		str,strlen(str));

	source=(char *)calloc((((width+7)/8)*height),1);
	before=XCreateImage(display,DefaultVisual(display,DefaultScreen(display)),
		1,XYPixmap,0,source,width,height,8,0);
	before->byte_order=before->bitmap_bit_order=MSBFirst;
	XGetSubImage(display,pix,0,0,width,height,1L,XYPixmap,before,0,0);
	source=(char *)calloc((((height+7)/8)*width),1);
	after=XCreateImage(display,DefaultVisual(display,DefaultScreen(display)),
		1,XYPixmap,0,source,height,width,8,0);
	after->byte_order=after->bitmap_bit_order=MSBFirst;

	for (yloop=0; yloop<height; yloop++) {
		for (xloop=0; xloop<width; xloop++) {
			source=before->data+(xloop/8)+
				(yloop*before->bytes_per_line);
			if (*source&(128>>(xloop%8))) {
				dest=after->data+(yloop/8)+
					((width-1-xloop)*after->bytes_per_line);
				*dest|=(128>>(yloop%8));
			}
		}
	}
	
#ifdef DEBUG_SCIPLOT_VTEXT
	if (1) {
	char sourcebit;
		for (yloop=0; yloop<before->height; yloop++) {
			for (xloop=0; xloop<before->width; xloop++) {
				source=before->data+(xloop/8)+
					(yloop*before->bytes_per_line);
				sourcebit=*source&(128>>(xloop%8));
				if (sourcebit) putchar('X');
				else putchar('.');
			}
			putchar('\n');
		}
	
		for (yloop=0; yloop<after->height; yloop++) {
			for (xloop=0; xloop<after->width; xloop++) {
				source=after->data+(xloop/8)+
					(yloop*after->bytes_per_line);
				sourcebit=*source&(128>>(xloop%8));
				if (sourcebit) putchar('X');
				else putchar('.');
			}
			putchar('\n');
		}
	}
#endif

	xdest=x-(int)FontAscent(f);
	if (xdest<0) xdest=0;
	ydest=y-width;

	XPutImage(display,rotpix,drawGC,after,0,0,0,0,
		after->width,after->height);
	
	XSetFillStyle(display,gc,FillStippled);
	XSetStipple(display,gc,rotpix);
	XSetTSOrigin(display,gc,xdest,ydest);
	XFillRectangle(display,win,gc,xdest,ydest,after->width,after->height);
	XSetFillStyle(display,gc,FillSolid);

	XFreeGC(display,drawGC);
	XDestroyImage(before);
	XDestroyImage(after);
	XFreePixmap(display,pix);
	XFreePixmap(display,rotpix);
}

static char dots[]={2,1,1},widedots[]={2,1,4};

static GC
ItemGetGC(w,item)
SciPlotWidget w;
SciPlotItem *item;
{
GC gc;
short color;

	switch (item->kind.any.style) {
	case XtLINE_SOLID:
		gc=w->plot.defaultGC;
		break;
	case XtLINE_DOTTED:
		XSetDashes(XtDisplay(w),w->plot.dashGC,0,&dots[1],
			(int)dots[0]);
		gc=w->plot.dashGC;
		break;
	case XtLINE_WIDEDOT:
		XSetDashes(XtDisplay(w),w->plot.dashGC,0,&widedots[1],
			(int)widedots[0]);
		gc=w->plot.dashGC;
		break;
	default:
		return NULL;
		break;
	}
	if (item->kind.any.color>=w->plot.num_colors)
		color=w->plot.ForegroundColor;
	else
		color=item->kind.any.color;
	XSetForeground(XtDisplay(w),gc,w->plot.colors[color]);
	return gc;
}

static GC
ItemGetFontGC(w,item)
SciPlotWidget w;
SciPlotItem *item;
{
GC gc;
short color,fontnum;

	gc=w->plot.dashGC;
	if (item->kind.any.color>=w->plot.num_colors)
		color=w->plot.ForegroundColor;
	else
		color=item->kind.any.color;
	XSetForeground(XtDisplay(w),gc,w->plot.colors[color]);
	if (item->kind.text.font>=w->plot.num_fonts) fontnum=0;
	else fontnum=item->kind.text.font;

/*
** fontnum==0 hack:  0 is supposed to be the default font, but the program
** can't seem to get the default font ID from the GC for some reason.  So,
** use a different GC where the default font still exists.
*/
	XSetFont(XtDisplay(w),gc,w->plot.fonts[fontnum].font->fid);
	return gc;
}

static void
ItemDraw(w,item)
SciPlotWidget w;
SciPlotItem *item;
{
XPoint point[8];
XSegment seg;
XRectangle rect;
int i;
GC gc;

	if (!XtIsRealized((Widget)w)) return;
	if ((item->type>SciPlotStartTextTypes)&&(item->type<SciPlotEndTextTypes))
		gc=ItemGetFontGC(w,item);
	else
		gc=ItemGetGC(w,item);
	if (!gc) return;
	switch (item->type) {
	case SciPlotLine:
		seg.x1=(short)item->kind.line.x1;
		seg.y1=(short)item->kind.line.y1;
		seg.x2=(short)item->kind.line.x2;
		seg.y2=(short)item->kind.line.y2;
		XDrawSegments(XtDisplay(w),XtWindow(w),gc,
			&seg,1);
		break;
	case SciPlotRect:
		XDrawRectangle(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.rect.x),
			(int)(item->kind.rect.y),
			(unsigned int)(item->kind.rect.w),
			(unsigned int)(item->kind.rect.h));
		break;
	case SciPlotFRect:
		XFillRectangle(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.rect.x),
			(int)(item->kind.rect.y),
			(unsigned int)(item->kind.rect.w),
			(unsigned int)(item->kind.rect.h));
		XDrawRectangle(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.rect.x),
			(int)(item->kind.rect.y),
			(unsigned int)(item->kind.rect.w),
			(unsigned int)(item->kind.rect.h));
		break;
	case SciPlotPoly:
		i=0;
		while (i<item->kind.poly.count) {
			point[i].x=(int)item->kind.poly.x[i];
			point[i].y=(int)item->kind.poly.y[i];
			i++;
		}
		point[i].x=(int)item->kind.poly.x[0];
		point[i].y=(int)item->kind.poly.y[0];
		XDrawLines(XtDisplay(w),XtWindow(w),gc,
			point,i+1,CoordModeOrigin);
		break;
	case SciPlotFPoly:
		i=0;
		while (i<item->kind.poly.count) {
			point[i].x=(int)item->kind.poly.x[i];
			point[i].y=(int)item->kind.poly.y[i];
			i++;
		}
		point[i].x=(int)item->kind.poly.x[0];
		point[i].y=(int)item->kind.poly.y[0];
		XFillPolygon(XtDisplay(w),XtWindow(w),gc,
			point,i+1,Complex,CoordModeOrigin);
		XDrawLines(XtDisplay(w),XtWindow(w),gc,
			point,i+1,CoordModeOrigin);
		break;
	case SciPlotCircle:
		XDrawArc(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.circ.x-item->kind.circ.r),
			(int)(item->kind.circ.y-item->kind.circ.r),
			(unsigned int)(item->kind.circ.r*2),
			(unsigned int)(item->kind.circ.r*2),
			0*64,360*64);
		break;
	case SciPlotFCircle:
		XFillArc(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.circ.x-item->kind.circ.r),
			(int)(item->kind.circ.y-item->kind.circ.r),
			(unsigned int)(item->kind.circ.r*2),
			(unsigned int)(item->kind.circ.r*2),
			0*64,360*64);
		break;
	case SciPlotText:
		XDrawString(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.text.x),(int)(item->kind.text.y),
			item->kind.text.text,
			(int)item->kind.text.length);
		break;
	case SciPlotVText:
		XDrawVString(XtDisplay(w),XtWindow(w),gc,
			(int)(item->kind.text.x),(int)(item->kind.text.y),
			item->kind.text.text,
			(int)item->kind.text.length,
			FontFromFontnum(w,item->kind.text.font));
		break;
	case SciPlotClipRegion:
		rect.x=(short)item->kind.line.x1;
		rect.y=(short)item->kind.line.y1;
		rect.width=(short)item->kind.line.x2;
		rect.height=(short)item->kind.line.y2;
		XSetClipRectangles(XtDisplay(w),w->plot.dashGC,0,0,&rect,1,Unsorted);
		XSetClipRectangles(XtDisplay(w),w->plot.defaultGC,0,0,&rect,1,Unsorted);
		break;
	case SciPlotClipClear:
		XSetClipMask(XtDisplay(w),w->plot.dashGC,None);
		XSetClipMask(XtDisplay(w),w->plot.defaultGC,None);
		break;
	default:
		break;
	}
}



/*
** PostScript (r) functions ------------------------------------------------
**
*/
typedef struct {
	char *command;
	char *prolog;
} PScommands;

static PScommands psc[]={
	{"ma",		"moveto"},
	{"da",		"lineto stroke newpath"},
	{"la",		"lineto"},
	{"poly",	"closepath stroke newpath"},
	{"fpoly",	"closepath fill newpath"},
	{"box",		"1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath stroke newpath"},
	{"fbox",	"1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath fill newpath"},
	{"clipbox",	"gsave 1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath clip newpath"},
	{"unclip",	"grestore"},
	{"cr",		"0 360 arc stroke newpath"},
	{"fcr",		"0 360 arc fill newpath"},
	{"vma",		"gsave moveto 90 rotate"},
	{"norm",	"grestore"},
	{"solid",	"[] 0 setdash"},
	{"dot",		"[.25 2] 0 setdash"},
	{"widedot",	"[.25 8] 0 setdash"},
	{NULL,NULL}
};

enum PSenums {
	PSmoveto,PSlineto,
	PSpolyline,PSendpoly,PSendfill,
	PSbox,PSfbox,
	PSclipbox,PSunclip,
	PScircle,PSfcircle,
	PSvmoveto,PSnormal,
	PSsolid,PSdot,PSwidedot
};

static void
ItemPSDrawAll(w,fd,yflip)
SciPlotWidget w;
FILE *fd;
float yflip;
{
int i,loopcount;
SciPlotItem	*item;
int previousfont,previousline,currentline;

	item=w->plot.drawlist;
	loopcount=0;
	previousfont=0;
	previousline=XtLINE_SOLID;
	while (loopcount<w->plot.num_drawlist) {

/* 2 switch blocks:  1st sets up defaults, 2nd actually draws things. */
		currentline=previousline;
		switch (item->type) {
		case SciPlotLine:
		case SciPlotCircle:
			currentline=item->kind.any.style;
			break;
		default:
			break;
	       	}
	       	if (currentline!=XtLINE_NONE) {
	       		if (currentline!=previousline) {
				switch (item->kind.any.style) {
					case XtLINE_SOLID:
	       					fprintf(fd,"%s ",psc[PSsolid].command);
						break;
					case XtLINE_DOTTED:
	       					fprintf(fd,"%s ",psc[PSdot].command);
						break;
					case XtLINE_WIDEDOT:
	       					fprintf(fd,"%s ",psc[PSwidedot].command);
						break;
				}
				previousline=currentline;
	       		}
	       		
			switch (item->type) {
			case SciPlotLine:
	       			fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
 					item->kind.line.x1,yflip-item->kind.line.y1,
 						psc[PSmoveto].command,
					item->kind.line.x2,yflip-item->kind.line.y2,
						psc[PSlineto].command);
				break;
			case SciPlotRect:
        			fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
 					item->kind.rect.x,
 					yflip-item->kind.rect.y-(item->kind.rect.h-1.0),
 						psc[PSmoveto].command,
					item->kind.rect.w-1.0,item->kind.rect.h-1.0,
						psc[PSbox].command);
				break;
			case SciPlotFRect:
        			fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
 					item->kind.rect.x,
 					yflip-item->kind.rect.y-(item->kind.rect.h-1.0),
 						psc[PSmoveto].command,
					item->kind.rect.w-1.0,item->kind.rect.h-1.0,
						psc[PSfbox].command);
				break;
			case SciPlotPoly:
				fprintf(fd,"%.2f %.2f %s ",
					item->kind.poly.x[0],yflip-item->kind.poly.y[0],
						psc[PSmoveto].command);
				for (i=1; i<item->kind.poly.count; i++) {
					fprintf(fd,"%.2f %.2f %s ",
 						item->kind.poly.x[i],
 						yflip-item->kind.poly.y[i],
							psc[PSpolyline].command);
				}
				fprintf(fd,"%s\n",psc[PSendpoly].command);
				break;
			case SciPlotFPoly:
				fprintf(fd,"%.2f %.2f %s ",
					item->kind.poly.x[0],yflip-item->kind.poly.y[0],
						psc[PSmoveto].command);
				for (i=1; i<item->kind.poly.count; i++) {
					fprintf(fd,"%.2f %.2f %s ",
 						item->kind.poly.x[i],
 						yflip-item->kind.poly.y[i],
							psc[PSpolyline].command);
				}
				fprintf(fd,"%s\n",psc[PSendfill].command);
				break;
			case SciPlotCircle:
        			fprintf(fd,"%.2f %.2f %.2f %s\n",
 					item->kind.circ.x,yflip-item->kind.circ.y,
 					item->kind.circ.r,
 						psc[PScircle].command);
				break;
			case SciPlotFCircle:
        			fprintf(fd,"%.2f %.2f %.2f %s\n",
 					item->kind.circ.x,yflip-item->kind.circ.y,
 					item->kind.circ.r,
 						psc[PSfcircle].command);
				break;
			case SciPlotText:
        			fprintf(fd,"font-%d %.2f %.2f %s (%s) show\n",
        				item->kind.text.font,
 					item->kind.text.x,yflip-item->kind.text.y,
 						psc[PSmoveto].command,
 					item->kind.text.text);
				break;
			case SciPlotVText:
        			fprintf(fd,"font-%d %.2f %.2f %s (%s) show %s\n",
        				item->kind.text.font,
 					item->kind.text.x,yflip-item->kind.text.y,
 						psc[PSvmoveto].command,
 					item->kind.text.text,
 						psc[PSnormal].command);
				break;
			case SciPlotClipRegion:
        			fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
 					item->kind.line.x1,
 					yflip-item->kind.line.y1-item->kind.line.y2,
 						psc[PSmoveto].command,
					item->kind.line.x2,item->kind.line.y2,
						psc[PSclipbox].command);
				break;
			case SciPlotClipClear:
				fprintf(fd,"%s\n",psc[PSunclip].command);
				break;
			default:
				break;
			}
		}
		loopcount++;
		item++;
	}
}

Boolean
SciPlotPSCreateFancy(w,filename,drawborder,titles)
SciPlotWidget w;
char *filename;
int drawborder;
char *titles;
{
FILE *fd;
float scale,xoff,yoff,xmax,ymax,yflip,aspect,border,titlefontsize;
int	i;
PScommands *p;
char fontname[128];

	if (!(fd = fopen(filename, "w"))) {
		XtWarning("Unable to open postscript file.");
		return False;
	}

	aspect=(float)w->core.width/(float)w->core.height;
	border=36.0;
	if (aspect>(612.0/792.0)) {
		scale=(612.0-(2*border))/(float)w->core.width;
		xoff=border;
		yoff=(792.0-(2*border)-scale*(float)w->core.height)/2.0;
		xmax=xoff+scale*(float)w->core.width;
		ymax=yoff+scale*(float)w->core.height;
	}
	else {
		scale=(792.0-(2*border))/(float)w->core.height;
		yoff=border;
		xoff=(612.0-(2*border)-scale*(float)w->core.width)/2.0;
		xmax=xoff+scale*(float)w->core.width;
		ymax=yoff+scale*(float)w->core.height;
	}
	yflip=w->core.height;
	fprintf(fd,"%s\n%s %.2f  %s\n%s %f %f %f %f\n%s\n",
		"%!PS-ADOBE-3.0 EPSF-3.0",
		"%%Creator: SciPlot Widget",
		_SCIPLOT_WIDGET_VERSION,
		"Copyright (c) 1994 Robert W. McMullen",
		"%%BoundingBox:",xoff,yoff,xmax,ymax,
		"%%EndComments");
	
	p=psc;
	while (p->command) {
		fprintf(fd,"/%s {%s} bind def\n",p->command,p->prolog);
		p++;
	}

	for (i=0; i<w->plot.num_fonts; i++) {
		FontnumPostScriptString(w,i,fontname,128);	/* last number here (128) is length of 'fontname' */
		fprintf(fd,"/font-%d {%s setfont} bind def\n",
			i,fontname);
	}
	titlefontsize=10.0;
	fprintf(fd,"/font-title {/%s findfont %f scalefont setfont} bind def\n",
		"Times-Roman",titlefontsize);
	fprintf(fd,"%f setlinewidth\n",0.001);
	fprintf(fd,"newpath gsave\n%f %f translate %f %f scale\n",
		xoff,yoff,scale,scale);

	ItemPSDrawAll(w,fd,yflip);

	fprintf(fd,"grestore\n");
	
	if (drawborder) {
		fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
			border,border,
				psc[PSmoveto].command,
			612.0-2.0*border,792.0-2.0*border,
				psc[PSbox].command);
	}
	if (titles) {
	char *ptr;
	char buf[256];
	int len,i,j;
	float x,y;
	
		x=border+titlefontsize;
		y=792.0-border-(2.0*titlefontsize);
		len=strlen(titles);
		ptr=titles;
		i=0;
		while (i<len) {
			j=0;
			while ((*ptr!='\n') && (i<len)) {
				if ((*ptr=='(')||(*ptr==')'))
					buf[j++]='\\';
				buf[j++]=*ptr;
				ptr++;
				i++;
			}
			buf[j]='\0';
			ptr++;
			i++;
        		fprintf(fd,"font-title %.2f %.2f %s (%s) show\n",
        			x,y,psc[PSmoveto].command,buf);
			y-=titlefontsize*1.5;
		}
		if (border) {
			y+=titlefontsize*0.5;
	       		fprintf(fd,"%.2f %.2f %s %.2f %.2f %s\n",
 				border,y,
 					psc[PSmoveto].command,
				612.0-border,y,
					psc[PSlineto].command);
		}
	}
	
	fprintf(fd,"showpage\n");
	fclose(fd);
	return True;
}

Boolean
SciPlotPSCreate(wi,filename)
Widget wi;
char *filename;
{
SciPlotWidget w;

	w=(SciPlotWidget)wi;
	return SciPlotPSCreateFancy(w,filename,False,NULL);
}


/*
** Private device independent drawing functions ----------------------------
**
*/

static void
EraseAll(w)
SciPlotWidget w;
{
SciPlotItem	*item;
int	i;

	item=w->plot.drawlist;
	i=0;
	while (i<w->plot.num_drawlist) {
		if ((item->type>SciPlotStartTextTypes)&&
		    (item->type<SciPlotEndTextTypes))
			XtFree(item->kind.text.text);
		i++;
		item++;
	}
	w->plot.num_drawlist=0;
	if (XtIsRealized((Widget)w)) XClearWindow(XtDisplay(w),XtWindow(w));
}

static SciPlotItem *
ItemGetNew(w)
SciPlotWidget w;
{
SciPlotItem	*item;

	w->plot.num_drawlist++;
	if (w->plot.num_drawlist>=w->plot.alloc_drawlist) {
		w->plot.alloc_drawlist+=NUMPLOTITEMEXTRA;
		w->plot.drawlist=(SciPlotItem *)XtRealloc((char *)w->plot.drawlist,
			w->plot.alloc_drawlist*sizeof(SciPlotItem));
		if (!w->plot.drawlist) {
			printf("Can't realloc memory for SciPlotItem list\n");
			exit(1);
		}
#ifdef DEBUG_SCIPLOT
		printf("Alloced #%d for drawlist\n",w->plot.alloc_drawlist);
#endif
	}
	item=w->plot.drawlist+(w->plot.num_drawlist-1);
	item->type=SciPlotFALSE;
	return item;
}


static void
LineSet(w,x1,y1,x2,y2,color,style)
SciPlotWidget w;
real x1,y1,x2,y2;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.line.x1=(real)x1;
	item->kind.line.y1=(real)y1;
	item->kind.line.x2=(real)x2;
	item->kind.line.y2=(real)y2;
	item->type=SciPlotLine;
	ItemDraw(w,item);
}

static void
RectSet(w,x1,y1,x2,y2,color,style)
SciPlotWidget w;
real x1,y1,x2,y2;
int color,style;
{
SciPlotItem	*item;
real x,y,width,height;

	if (x1<x2) x=x1,width=(x2-x1+1);
	else x=x2,width=(x1-x2+1);
	if (y1<y2) y=y1,height=(y2-y1+1);
	else y=y2,height=(y1-y2+1);

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.rect.x=(real)x;
	item->kind.rect.y=(real)y;
	item->kind.rect.w=(real)width;
	item->kind.rect.h=(real)height;
	item->type=SciPlotRect;
	ItemDraw(w,item);
}

static void
FilledRectSet(w,x1,y1,x2,y2,color,style)
SciPlotWidget w;
real x1,y1,x2,y2;
int color,style;
{
SciPlotItem	*item;
real x,y,width,height;

	if (x1<x2) x=x1,width=(x2-x1+1);
	else x=x2,width=(x1-x2+1);
	if (y1<y2) y=y1,height=(y2-y1+1);
	else y=y2,height=(y1-y2+1);

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.rect.x=(real)x;
	item->kind.rect.y=(real)y;
	item->kind.rect.w=(real)width;
	item->kind.rect.h=(real)height;
	item->type=SciPlotFRect;
	ItemDraw(w,item);
}

static void
TriSet(w,x1,y1,x2,y2,x3,y3,color,style)
SciPlotWidget w;
real x1,y1,x2,y2,x3,y3;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.poly.count=3;
	item->kind.poly.x[0]=(real)x1;
	item->kind.poly.y[0]=(real)y1;
	item->kind.poly.x[1]=(real)x2;
	item->kind.poly.y[1]=(real)y2;
	item->kind.poly.x[2]=(real)x3;
	item->kind.poly.y[2]=(real)y3;
	item->type=SciPlotPoly;
	ItemDraw(w,item);
}

static void
FilledTriSet(w,x1,y1,x2,y2,x3,y3,color,style)
SciPlotWidget w;
real x1,y1,x2,y2,x3,y3;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.poly.count=3;
	item->kind.poly.x[0]=(real)x1;
	item->kind.poly.y[0]=(real)y1;
	item->kind.poly.x[1]=(real)x2;
	item->kind.poly.y[1]=(real)y2;
	item->kind.poly.x[2]=(real)x3;
	item->kind.poly.y[2]=(real)y3;
	item->type=SciPlotFPoly;
	ItemDraw(w,item);
}

static void
QuadSet(w,x1,y1,x2,y2,x3,y3,x4,y4,color,style)
SciPlotWidget w;
real x1,y1,x2,y2,x3,y3,x4,y4;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.poly.count=4;
	item->kind.poly.x[0]=(real)x1;
	item->kind.poly.y[0]=(real)y1;
	item->kind.poly.x[1]=(real)x2;
	item->kind.poly.y[1]=(real)y2;
	item->kind.poly.x[2]=(real)x3;
	item->kind.poly.y[2]=(real)y3;
	item->kind.poly.x[3]=(real)x4;
	item->kind.poly.y[3]=(real)y4;
	item->type=SciPlotPoly;
	ItemDraw(w,item);
}

static void
FilledQuadSet(w,x1,y1,x2,y2,x3,y3,x4,y4,color,style)
SciPlotWidget w;
real x1,y1,x2,y2,x3,y3,x4,y4;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.poly.count=4;
	item->kind.poly.x[0]=(real)x1;
	item->kind.poly.y[0]=(real)y1;
	item->kind.poly.x[1]=(real)x2;
	item->kind.poly.y[1]=(real)y2;
	item->kind.poly.x[2]=(real)x3;
	item->kind.poly.y[2]=(real)y3;
	item->kind.poly.x[3]=(real)x4;
	item->kind.poly.y[3]=(real)y4;
	item->type=SciPlotFPoly;
	ItemDraw(w,item);
}

static void
CircleSet(w,x,y,r,color,style)
SciPlotWidget w;
real x,y,r;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.circ.x=(real)x;
	item->kind.circ.y=(real)y;
	item->kind.circ.r=(real)r;
	item->type=SciPlotCircle;
	ItemDraw(w,item);
}

static void
FilledCircleSet(w,x,y,r,color,style)
SciPlotWidget w;
real x,y,r;
int color,style;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=(short)style;
	item->kind.circ.x=(real)x;
	item->kind.circ.y=(real)y;
	item->kind.circ.r=(real)r;
	item->type=SciPlotFCircle;
	ItemDraw(w,item);
}

static void
TextSet(w,x,y,text,color,font)
SciPlotWidget w;
real x,y;
char *text;
int color,font;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=0;
	item->kind.text.x=(real)x;
	item->kind.text.y=(real)y;
	item->kind.text.length=strlen(text);
	item->kind.text.text=XtMalloc((int)item->kind.text.length+1);
	item->kind.text.font=font;
	strcpy(item->kind.text.text,text);
	item->type=SciPlotText;
	ItemDraw(w,item);
#ifdef DEBUG_SCIPLOT_TEXT
	if (1) {
	real x1,y1;
	
		y-=FontnumAscent(w,font);
		y1=y+FontnumHeight(w,font)-1.0;
		x1=x+FontnumTextWidth(w,font,text)-1.0;
		RectSet(w,x,y,x1,y1,color,XtLINE_SOLID);
	}
#endif
}

static void
TextCenter(w,x,y,text,color,font)
SciPlotWidget w;
real x,y;
char *text;
int color,font;
{
	x-=FontnumTextWidth(w,font,text)/2.0;
	y+=FontnumHeight(w,font)/2.0 - FontnumDescent(w,font);
	TextSet(w,x,y,text,color,font);
}

static void
VTextSet(w,x,y,text,color,font)
SciPlotWidget w;
real x,y;
char *text;
int color,font;
{
SciPlotItem	*item;

	item=ItemGetNew(w);
	item->kind.any.color=(short)color;
	item->kind.any.style=0;
	item->kind.text.x=(real)x;
	item->kind.text.y=(real)y;
	item->kind.text.length=strlen(text);
	item->kind.text.text=XtMalloc((int)item->kind.text.length+1);
	item->kind.text.font=font;
	strcpy(item->kind.text.text,text);
	item->type=SciPlotVText;
	ItemDraw(w,item);
#ifdef DEBUG_SCIPLOT_TEXT
	if (1) {
	real x1,y1;

		x+=FontnumDescent(w,font);
		x1=x-FontnumHeight(w,font)-1.0;
		y1=y-FontnumTextWidth(w,font,text)-1.0;
		RectSet(w,x,y,x1,y1,color,XtLINE_SOLID);
	}
#endif
}

static void
VTextCenter(w,x,y,text,color,font)
SciPlotWidget w;
real x,y;
char *text;
int color,font;
{
	x+=FontnumHeight(w,font)/2.0 - FontnumDescent(w,font);
	y+=FontnumTextWidth(w,font,text)/2.0;
	VTextSet(w,x,y,text,color,font);
}

static void
ClipSet(w)
SciPlotWidget w;
{
SciPlotItem	*item;

	if (w->plot.ChartType==XtCARTESIAN) {
		item=ItemGetNew(w);
		item->kind.any.style=XtLINE_SOLID;
		item->kind.any.color=1;
		item->kind.line.x1=w->plot.x.Origin;
		item->kind.line.x2=w->plot.x.Size;
		item->kind.line.y1=w->plot.y.Origin;
		item->kind.line.y2=w->plot.y.Size;
#ifdef DEBUG_SCIPLOT
		printf("clipping region: x=%f y=%f w=%f h=%f\n",
			item->kind.line.x1,
			item->kind.line.y1,
			item->kind.line.x2,
			item->kind.line.y2
		);
#endif
		item->type=SciPlotClipRegion;
		ItemDraw(w,item);
	}
}

static void
ClipClear(w)
SciPlotWidget w;
{
SciPlotItem	*item;

	if (w->plot.ChartType==XtCARTESIAN) {
		item=ItemGetNew(w);
		item->kind.any.style=XtLINE_SOLID;
		item->kind.any.color=1;
		item->type=SciPlotClipClear;
		ItemDraw(w,item);
	}
}


/*
** Private List functions -------------------------------------------------
**
*/

static int
List_New(w)
SciPlotWidget w;
{
int index;
SciPlotList *p;
Boolean found;

/* First check to see if there is any free space in the index */
	found=FALSE;
	for (index=0; index<w->plot.num_plotlist; index++) {
		p=w->plot.plotlist+index;
		if (!p->used) {
			found=TRUE;
			break;
		}
	}
	
/* If no space is found, increase the size of the index */
	if (!found) {
		w->plot.num_plotlist++;
		if (w->plot.alloc_plotlist==0) {
			w->plot.alloc_plotlist=NUMPLOTLINEALLOC;
			w->plot.plotlist=(SciPlotList *)XtCalloc(w->plot.alloc_plotlist,sizeof(SciPlotList));
			if (!w->plot.plotlist) {
				printf("Can't calloc memory for SciPlotList\n");
				exit(1);
			}
			w->plot.alloc_plotlist=NUMPLOTLINEALLOC;
		}
		else if (w->plot.num_plotlist>w->plot.alloc_plotlist) {
			w->plot.alloc_plotlist+=NUMPLOTLINEALLOC;
			w->plot.plotlist=(SciPlotList *)XtRealloc((char *)w->plot.plotlist,
			w->plot.alloc_plotlist*sizeof(SciPlotList));
			if (!w->plot.plotlist) {
				printf("Can't realloc memory for SciPlotList\n");
				exit(1);
			}
		}
		index=w->plot.num_plotlist-1;
		p=w->plot.plotlist+index;
	}
	
	p->LineStyle=p->LineColor=p->PointStyle=p->PointColor=0;
	p->number=p->allocated=0;
	p->data=NULL;
	p->legend=NULL;
	p->draw=p->used=TRUE;
	return index;
}

static void
List_Delete(p)
SciPlotList *p;
{
	p->draw=p->used=FALSE;
	p->number=p->allocated=0;
	if (p->data) free(p->data);
	p->data=NULL;
	if (p->legend) free(p->legend);
	p->legend=NULL;
}

static SciPlotList *
List_Find(w,id)
SciPlotWidget w;
int id;
{
SciPlotList *p;

	if ((id>=0)&&(id<w->plot.num_plotlist)) {
		p=w->plot.plotlist+id;
		if (p->used) return p;
	}
	return NULL;
}

static void
List_SetStyle(p,pcolor,pstyle,lcolor,lstyle)
SciPlotList *p;
int pstyle,pcolor,lstyle,lcolor;
{
/* Note!  Do checks in here later on... */

	if (lstyle>=0) p->LineStyle=lstyle;
	if (lcolor>=0) p->LineColor=lcolor;
	if (pstyle>=0) p->PointStyle=pstyle;
	if (pcolor>=0) p->PointColor=pcolor;
}

static void
List_SetLegend(p,legend)
SciPlotList *p;
char *legend;
{
/* Note!  Do checks in here later on... */

	p->legend=(char *)XtMalloc(strlen(legend)+1);
	strcpy(p->legend,legend);
}

static void
List_AllocData(p,num)
SciPlotList *p;
int num;
{
	if (p->data) {
		free(p->data);
		p->allocated=0;
	}
	p->allocated=num+NUMPLOTDATAEXTRA;
	p->data=(realpair *)XtCalloc(p->allocated,sizeof(realpair));
	if (!p->data) {
		p->number=p->allocated=0;
	}
}

static void
List_SetReal(p,num,xlist,ylist)
SciPlotList *p;
int num;
real *xlist,*ylist;
{
int i;

	if ((!p->data)||(p->allocated<num)) List_AllocData(p,num);
	if (p->data) {
		p->number=num;
		for (i=0; i<num; i++) {
			p->data[i].x=xlist[i];
			p->data[i].y=ylist[i];
		}
	}
}

static void
List_SetFloat(p,num,xlist,ylist)
SciPlotList *p;
int num;
float *xlist,*ylist;
{
int i;

	if ((!p->data)||(p->allocated<num)) List_AllocData(p,num);
	if (p->data) {
		p->number=num;
		for (i=0; i<num; i++) {
			p->data[i].x=(real)xlist[i];
			p->data[i].y=(real)ylist[i];
		}
	}
}

static void
List_SetDouble(p,num,xlist,ylist)
SciPlotList *p;
int num;
double *xlist,*ylist;
{
int i;

	if ((!p->data)||(p->allocated<num)) List_AllocData(p,num);
	if (p->data) {
		p->number=num;
		for (i=0; i<num; i++) {
			p->data[i].x=(real)xlist[i];
			p->data[i].y=(real)ylist[i];
		}
	}
}


/*
** Private data point to screen location converters -------------------------
**
*/

static real
PlotX(w,xin)
SciPlotWidget w;
real xin;
{
real xout;

	xin *= w->plot.x.Scalefact;
	if (w->plot.XLog) xout=w->plot.x.Origin +
		((log10(xin) - log10(w->plot.x.DrawOrigin)) *
		(w->plot.x.Size / w->plot.x.DrawSize));
	else xout=w->plot.x.Origin +
		((xin - w->plot.x.DrawOrigin*w->plot.x.Scalefact) *
		(w->plot.x.Size / (w->plot.x.DrawSize*w->plot.x.Scalefact)));
	return xout;
}

static real
PlotY(w,yin)
SciPlotWidget w;
real yin;
{
real yout;

	yin *= w->plot.y.Scalefact;
	if (w->plot.YLog) {
		yout=w->plot.y.Origin + w->plot.y.Size -
			((log10(yin) - log10(w->plot.y.DrawOrigin)) *
			(w->plot.y.Size / w->plot.y.DrawSize));
		}
	else yout=w->plot.y.Origin + w->plot.y.Size -
		((yin - w->plot.y.DrawOrigin*w->plot.y.Scalefact) *
		(w->plot.y.Size / (w->plot.y.DrawSize*w->plot.y.Scalefact)));

	return yout;
}

static void
PlotRTRadians(w,r,t,xout,yout)
SciPlotWidget w;
real r,t,*xout,*yout;
{
	*xout=w->plot.x.Center + (r*(real)cos(t) /
		w->plot.PolarScale*w->plot.x.Size/2.0);
	*yout=w->plot.y.Center + (-r*(real)sin(t) /
		w->plot.PolarScale*w->plot.x.Size/2.0);
}

static void
PlotRTDegrees(w,r,t,xout,yout)
SciPlotWidget w;
real r,t,*xout,*yout;
{
	t*=DEG2RAD;
	PlotRTRadians(w,r,t,xout,yout);
}

static void
PlotRT(w,r,t,xout,yout)
SciPlotWidget w;
real r,t,*xout,*yout;
{
	if (w->plot.Degrees) t*=DEG2RAD;
	PlotRTRadians(w,r,t,xout,yout);
}


/*
** Private calculation utilities for axes ---------------------------------
**
*/

#define NUMBER_MINOR	8
#define MAX_MAJOR	8
static float CAdeltas[8]={0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0};
static int CAdecimals[8]={  0,   0,    1,   0,   0,   0,   1,   0};
static int CAminors[8]  ={  4,   4,    4,   5,   4,   4,   4,   5};

static void
ComputeAxis(axis,min,max,log)
SciPlotAxis *axis;
real min,max;
Boolean log;
{ 
double range,rnorm,delta,calcmin,calcmax,base,expon,maxabs;
int nexp,majornum,minornum,majordecimals,decimals,i;

	axis->Scalefact = 1.0;
	axis->Scale_expon = 0;
	range=max-min;
	if (log) {
		calcmin=pow(10.0,(double)((int)floor(log10(min))));
		calcmax=pow(10.0,(double)((int)ceil(log10(max))));
		delta=10.0;
		
		axis->DrawOrigin=(real)calcmin;
		axis->DrawMax=(real)calcmax;
		axis->DrawSize=(real)(log10(calcmax)-log10(calcmin));
		axis->MajorInc=(real)delta;
		axis->MajorNum=(int)(log10(calcmax)-log10(calcmin))+1;
		axis->MinorNum=10;
		axis->Precision=-(int)(log10(calcmin)*1.0001);
#ifdef DEBUG_SCIPLOT
	printf("calcmin=%e log=%e (int)log=%d  Precision=%d\n",
		calcmin,log10(calcmin),(int)(log10(calcmin)*1.0001),axis->Precision);
#endif
		if (axis->Precision<0) axis->Precision=0;
	}
	else {
		nexp=(int)(floor(log10(range)));
		rnorm=range/pow(10.0,(double)nexp);
		for (i=0; i<NUMBER_MINOR; i++) {
			delta=CAdeltas[i];
			minornum=CAminors[i];
			majornum=(int)((rnorm+0.9999999*delta)/delta);
			majordecimals=CAdecimals[i];
			if (majornum<=MAX_MAJOR) break;
		}
		delta*=pow(10.0,(double)nexp);
#ifdef DEBUG_SCIPLOT
	printf("nexp=%d range=%f rnorm=%f delta=%f\n",nexp,range,rnorm,delta);
#endif

		if (min<0.0)
			calcmin=((double)((int)((min-.9999999*delta)/delta)))*delta;
		else if ((min>0.0)&&(min<1.0))
			calcmin=((double)((int)((1.0000001*min)/delta)))*delta;
		else if (min>=1.0)
			calcmin=((double)((int)((.9999999*min)/delta)))*delta;
		else
			calcmin=min;
		if (max<0.0)
			calcmax=((double)((int)((.9999999*max)/delta)))*delta;
		else if (max>0.0)
			calcmax=((double)((int)((max+.9999999*delta)/delta)))*delta;
		else
			calcmax=max;
		
maxabs = (fabs(calcmax) > fabs(calcmin)) ? fabs(calcmax) : fabs(calcmin);
expon = log10( maxabs );
expon = my_nint(expon);
base = 10.0;
expon = -1.0*expon;
if( expon > 3.0 )
	axis->Scalefact = pow( base, expon );
else if( expon < -3.0 ) 
	{
	expon += 1.0;
	axis->Scalefact = pow( base, expon );
	}
else
	expon = 0.0;
axis->Scale_expon = my_nint(expon);
		axis->DrawOrigin=(real)calcmin;
		axis->DrawMax=(real)calcmax;
		axis->DrawSize=(real)(calcmax-calcmin);
		axis->MajorInc=(real)delta;
		axis->MajorNum=majornum;
		axis->MinorNum=minornum;

		delta=log10(axis->MajorInc*axis->Scalefact);
		if (delta>0.0)
			decimals=-(int)floor(delta)+majordecimals;
		else
			decimals=(int)ceil(-delta)+majordecimals;
		if (decimals<0) decimals=0;
#ifdef DEBUG_SCIPLOT
	printf("delta=%f majordecimals=%d decimals=%d\n",
		delta,majordecimals,decimals);
#endif
		axis->Precision=decimals;
	}
	
#ifdef DEBUG_SCIPLOT
	printf("Tics: min=%f max=%f size=%f  major inc=%f #major=%d #minor=%d decimals=%d\n",
		axis->DrawOrigin,axis->DrawMax,axis->DrawSize,
		axis->MajorInc,axis->MajorNum,axis->MinorNum,axis->Precision);
#endif
}

static void
ComputeDrawingRange(w)
SciPlotWidget w;
{
	if (w->plot.ChartType==XtCARTESIAN) {
		ComputeAxis(&w->plot.x,w->plot.Min.x,w->plot.Max.x,
			w->plot.XLog);
		ComputeAxis(&w->plot.y,w->plot.Min.y,w->plot.Max.y,
			w->plot.YLog);
	}
	else {
		ComputeAxis(&w->plot.x,(real)0.0,w->plot.Max.x,
			(Boolean)FALSE);
		w->plot.PolarScale=w->plot.x.DrawMax;
	}
}

static void
ComputeMinMax(w)
SciPlotWidget w;
{
register int i,j;
register SciPlotList *p;
register real val;
Boolean firstx,firsty;

	w->plot.Min.x=w->plot.Min.y=w->plot.Max.x=w->plot.Max.y=1.0;
	firstx=True;
	firsty=True;
	
	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
			for (j=0; j<p->number; j++) {
				val=p->data[j].x;
				if (!w->plot.XLog||(w->plot.XLog&&(val>0.0))) {
					if (firstx) {
						w->plot.Min.x=w->plot.Max.x=val;
						firstx=False;
					}
					else {
						if (val>w->plot.Max.x)
							w->plot.Max.x=val;
						else if (val<w->plot.Min.x)
							w->plot.Min.x=val;
					}
				}

				val=p->data[j].y;
				if (!w->plot.YLog||(w->plot.YLog&&(val>0.0))) {
					if (firsty) {
						w->plot.Min.y=w->plot.Max.y=val;
						firsty=False;
					}
					else {
						if (val>w->plot.Max.y)
							w->plot.Max.y=val;
						else if (val<w->plot.Min.y)
							w->plot.Min.y=val;
					}
				}
			}
		}
	}
	if (firstx) {
		if (w->plot.XLog) {
			w->plot.Min.x=1.0;
			w->plot.Max.x=10.0;
		}
		else {
			w->plot.Min.x=0.0;
			w->plot.Max.x=10.0;
		}
	}
	if (firsty) {
		if (w->plot.YLog) {
			w->plot.Min.y=1.0;
			w->plot.Max.y=10.0;
		}
		else {
			w->plot.Min.y=0.0;
			w->plot.Max.y=10.0;
		}
	}
	if (w->plot.ChartType==XtCARTESIAN) {
		if (!w->plot.XLog) {
			if (!w->plot.XAutoScale) {
				w->plot.Min.x=w->plot.UserMin.x;
				w->plot.Max.x=w->plot.UserMax.x;
			}
			else if (w->plot.XOrigin) {
				if (w->plot.Min.x>0.0) w->plot.Min.x=0.0;
				if (w->plot.Max.x<0.0) w->plot.Max.x=0.0;
			}
		}
		if (!w->plot.YLog) {
			if (!w->plot.YAutoScale) {
				w->plot.Min.y=w->plot.UserMin.y;
				w->plot.Max.y=w->plot.UserMax.y;
			}
			else if (w->plot.YOrigin) {
				if (w->plot.Min.y>0.0) w->plot.Min.y=0.0;
				if (w->plot.Max.y<0.0) w->plot.Max.y=0.0;
			}
		}
	}
	else {
		if (fabs(w->plot.Min.x)>fabs(w->plot.Max.x))
			w->plot.Max.x=fabs(w->plot.Min.x);
	}
	
#ifdef DEBUG_SCIPLOT
printf("Min: (%f,%f)\tMax: (%f,%f)\n",
	w->plot.Min.x,w->plot.Min.y,
	w->plot.Max.x,w->plot.Max.y);
#endif
}

static void
ComputeLegendDimensions(w)
SciPlotWidget w;
{
real current,xmax,ymax;
int i;
SciPlotList *p;

	if (w->plot.ShowLegend) {
		xmax=0.0;
		ymax=2.0*(real)w->plot.LegendMargin;
		
		for (i=0; i<w->plot.num_plotlist; i++) {
			p=w->plot.plotlist+i;
			if (p->draw) {
				current=(real)w->plot.Margin +
					(real)w->plot.LegendMargin*3.0 +
					(real)w->plot.LegendLineSize +
					FontnumTextWidth(w,w->plot.axisFont,p->legend);
				if (current>xmax) xmax=current;
				ymax+=FontnumHeight(w,w->plot.axisFont);
			}
		}
		
		w->plot.x.LegendSize=xmax;
		w->plot.x.LegendPos=(real)w->plot.Margin;
		w->plot.y.LegendSize=ymax;
		w->plot.y.LegendPos=0.0;
	}
	else {
		w->plot.x.LegendSize=
		w->plot.x.LegendPos=
		w->plot.y.LegendSize=
		w->plot.y.LegendPos=0.0;
	}
}

static void
ComputeDimensions(w)
SciPlotWidget w;
{
real x,y,width,height,axisnumbersize,axisXlabelsize,axisYlabelsize;

/* x,y is the origin of the upper left corner of the drawing area inside
** the widget.  Doesn't necessarily have to be (Margin,Margin) as it is now.
*/
	x=(real)w->plot.Margin;
	y=(real)w->plot.Margin;

/* width = (real)w->core.width - (real)w->plot.Margin - x -
**		legendwidth - AxisFontHeight
*/
	width=(real)w->core.width - (real)w->plot.Margin - x -
		w->plot.x.LegendSize;

/* height = (real)w->core.height - (real)w->plot.Margin - y
**		- Height of axis numbers (including margin)
**		- Height of axis label (including margin)
**		- Height of Title (including margin)
*/
	height=(real)w->core.height - (real)w->plot.Margin - y;

	w->plot.x.Origin=x;
	w->plot.y.Origin=y;

/* Adjust the size depending upon what sorts of text are visible. */
	if (w->plot.ShowTitle)
		height-=(real)w->plot.TitleMargin+
			FontnumHeight(w,w->plot.titleFont);

	if (w->plot.ChartType==XtCARTESIAN) {
		axisnumbersize=(real)w->plot.Margin+
			FontnumHeight(w,w->plot.axisFont);
		height-=axisnumbersize;
		width-=axisnumbersize;
		w->plot.x.Origin+=axisnumbersize;
		
		if (w->plot.ShowXLabel) {
			axisXlabelsize=(real)w->plot.Margin+
				FontnumHeight(w,w->plot.labelFont);
			height-=axisXlabelsize;
		}
		if (w->plot.ShowYLabel) {
			axisYlabelsize=(real)w->plot.Margin+
				FontnumHeight(w,w->plot.labelFont);
			width-=axisYlabelsize;
			w->plot.x.Origin+=axisYlabelsize;
		}
	}
	
	w->plot.x.Size=width;
	w->plot.y.Size=height;

/* Adjust parameters for polar plot */
	if (w->plot.ChartType==XtPOLAR) {
		if (height<width) w->plot.x.Size=height;
	}
	w->plot.x.Center=w->plot.x.Origin+(width/2.0);
	w->plot.y.Center=w->plot.y.Origin+(height/2.0);
	
}

static void
AdjustDimensions(w)
SciPlotWidget w;
{
real xextra,yextra,val;
real x,y,width,height,axisnumbersize,axisXlabelsize,axisYlabelsize;
char numberformat[160],label[160];
int precision;

/* Compute xextra and yextra, which are the extra distances that the text
** labels on the axes stick outside of the graph.
*/
	xextra=yextra=0.0;
	if (w->plot.ChartType==XtCARTESIAN) {
		precision=w->plot.x.Precision;
		if (w->plot.XLog) {
			val=w->plot.x.DrawMax;
			precision-=w->plot.x.MajorNum;
			if (precision<0) precision=0;
		}
		else
			val=w->plot.x.DrawOrigin+floor(w->plot.x.DrawSize/
				w->plot.x.MajorInc)*w->plot.x.MajorInc;
		x=PlotX(w,val);
		val /= w->plot.x.Scalefact;
		get_number_format( numberformat, 160, precision, val );
		snprintf(label,160,numberformat,val);
		x+=FontnumTextWidth(w,w->plot.axisFont,label);
		if ((int)x>w->core.width) {
			xextra=ceil(x-w->core.width+w->plot.Margin);
			if (xextra<0.0) xextra=0.0;
		}

		precision=w->plot.y.Precision;
		if (w->plot.YLog) {
			val=w->plot.y.DrawMax;
			precision-=w->plot.y.MajorNum;
			if (precision<0) precision=0;
		}
		else
			val=w->plot.y.DrawOrigin+floor(w->plot.y.DrawSize/
				w->plot.y.MajorInc*1.0001)*w->plot.y.MajorInc;
		y=PlotY(w,val);
		val /= w->plot.y.Scalefact;
		get_number_format( numberformat, 160, precision, val );
		snprintf(label,160,numberformat,val);
#ifdef DEBUG_SCIPLOT
		printf("ylabel=%s\n",label);
#endif
		y-=FontnumTextWidth(w,w->plot.axisFont,label);
		if ((int)y<=0) {
			yextra=ceil(w->plot.Margin-y);
			if (yextra<0.0) yextra=0.0;
		}
	}
	else {
		val=w->plot.PolarScale;
		PlotRTDegrees(w,val,0.0,&x,&y);
		get_number_format( numberformat, 160, w->plot.x.Precision, val );
		snprintf(label,160,numberformat,val);
		x+=FontnumTextWidth(w,w->plot.axisFont,label);
		if ((int)x>w->core.width) {
			xextra=x-w->core.width+w->plot.Margin;
			if (xextra<0.0) xextra=0.0;
		}
		yextra=0.0;
	}


/* x,y is the origin of the upper left corner of the drawing area inside
** the widget.  Doesn't necessarily have to be (Margin,Margin) as it is now.
*/
	x=(real)w->plot.Margin;
	y=(real)w->plot.Margin+yextra;

/* width = (real)w->core.width - (real)w->plot.Margin - x -
**		legendwidth - AxisFontHeight
*/
	width=(real)w->core.width - (real)w->plot.Margin - x - xextra -
		w->plot.x.LegendSize;

/* height = (real)w->core.height - (real)w->plot.Margin - y
**		- Height of axis numbers (including margin)
**		- Height of axis label (including margin)
**		- Height of Title (including margin)
*/
	height=(real)w->core.height - (real)w->plot.Margin - y;

	w->plot.x.Origin=x;
	w->plot.y.Origin=y;

/* Adjust the size depending upon what sorts of text are visible. */
	if (w->plot.ShowTitle)
		height-=(real)w->plot.TitleMargin+
			FontnumHeight(w,w->plot.titleFont);

	axisnumbersize=0.0;
	axisXlabelsize=0.0;
	axisYlabelsize=0.0;
	if (w->plot.ChartType==XtCARTESIAN) {
		axisnumbersize=(real)w->plot.Margin+
			FontnumHeight(w,w->plot.axisFont);
		height-=axisnumbersize;
		width-=axisnumbersize;
		w->plot.x.Origin+=axisnumbersize;
		
		if (w->plot.ShowXLabel) {
			axisXlabelsize=(real)w->plot.Margin+
				FontnumHeight(w,w->plot.labelFont);
			height-=axisXlabelsize;
		}
		if (w->plot.ShowYLabel) {
			axisYlabelsize=(real)w->plot.Margin+
				FontnumHeight(w,w->plot.labelFont);
			width-=axisYlabelsize;
			w->plot.x.Origin+=axisYlabelsize;
		}
	}
	
	w->plot.x.Size=width;
	w->plot.y.Size=height;

/* Move legend position to the right of the plot */
	w->plot.x.LegendPos+=w->plot.x.Origin+w->plot.x.Size;
	w->plot.y.LegendPos+=w->plot.y.Origin;
	
/* Adjust parameters for polar plot */
	if (w->plot.ChartType==XtPOLAR) {
		if (height<width) w->plot.x.Size=height;
	}
	w->plot.x.Center=w->plot.x.Origin+(width/2.0);
	w->plot.y.Center=w->plot.y.Origin+(height/2.0);
	
	w->plot.y.AxisPos=w->plot.y.Origin+w->plot.y.Size+
		(real)w->plot.Margin+
		FontnumAscent(w,w->plot.axisFont);
	w->plot.x.AxisPos=w->plot.x.Origin-
		(real)w->plot.Margin-
		FontnumDescent(w,w->plot.axisFont);
	
	w->plot.y.LabelPos=w->plot.y.Origin+w->plot.y.Size+
		axisnumbersize+(real)w->plot.Margin+
		(FontnumHeight(w,w->plot.labelFont)/2.0);
	w->plot.x.LabelPos=w->plot.x.Origin-
		axisnumbersize-(real)w->plot.Margin-
		(FontnumHeight(w,w->plot.labelFont)/2.0);
	
	w->plot.y.TitlePos=w->plot.y.Origin+w->plot.y.Size+
		axisnumbersize+axisXlabelsize+(real)w->plot.TitleMargin+
		FontnumAscent(w,w->plot.titleFont);
	w->plot.x.TitlePos=x;
	
#ifdef DEBUG_SCIPLOT
	printf("y.Origin:		%f\n",w->plot.y.Origin);
	printf("y.Size:			%f\n",w->plot.y.Size);
	printf("axisnumbersize:		%f\n",axisnumbersize);
	printf("y.axisLabelSize:	%f\n",axisYlabelsize);
	printf("y.TitleSize:		%f\n",
		(real)w->plot.TitleMargin+FontnumHeight(w,w->plot.titleFont));
	printf("y.Margin:		%f\n",(real)w->plot.Margin);
	printf("total-------------------%f\n",w->plot.y.Origin+w->plot.y.Size+
		axisnumbersize+axisYlabelsize+(real)w->plot.Margin+
		(real)w->plot.TitleMargin+FontnumHeight(w,w->plot.titleFont));
	printf("total should be---------%f\n",(real)w->core.height);
#endif
}

static void
ComputeAllDimensions(w)
SciPlotWidget w;
{
	ComputeLegendDimensions(w);
	ComputeDimensions(w);
	ComputeDrawingRange(w);
	AdjustDimensions(w);
}

static void
ComputeAll(w)
SciPlotWidget w;
{
	ComputeMinMax(w);
	ComputeAllDimensions(w);
}


/*
** Private drawing routines -------------------------------------------------
**
*/

static void
DrawMarker(w,xpaper,ypaper,size,color,style)
SciPlotWidget w;
real xpaper,ypaper,size;
int style,color;
{
real sizex,sizey;

	switch(style) {
	case XtMARKER_CIRCLE:
		CircleSet(w,xpaper,ypaper,size,color,XtLINE_SOLID);
		break;
	case XtMARKER_FCIRCLE:
		FilledCircleSet(w,xpaper,ypaper,size,color,XtLINE_SOLID);
		break;
	case XtMARKER_SQUARE:
		size-=.5;
		RectSet(w,xpaper-size,ypaper-size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FSQUARE:
		size-=.5;
		FilledRectSet(w,xpaper-size,ypaper-size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_UTRIANGLE:
		sizex=size*.866;
		sizey=size/2.0;
		TriSet(w,xpaper,ypaper-size,
			xpaper+sizex,ypaper+sizey,
			xpaper-sizex,ypaper+sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FUTRIANGLE:
		sizex=size*.866;
		sizey=size/2.0;
		FilledTriSet(w,xpaper,ypaper-size,
			xpaper+sizex,ypaper+sizey,
			xpaper-sizex,ypaper+sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_DTRIANGLE:
		sizex=size*.866;
		sizey=size/2.0;
		TriSet(w,xpaper,ypaper+size,
			xpaper+sizex,ypaper-sizey,
			xpaper-sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FDTRIANGLE:
		sizex=size*.866;
		sizey=size/2.0;
		FilledTriSet(w,xpaper,ypaper+size,
			xpaper+sizex,ypaper-sizey,
			xpaper-sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_RTRIANGLE:
		sizey=size*.866;
		sizex=size/2.0;
		TriSet(w,xpaper+size,ypaper,
			xpaper-sizex,ypaper+sizey,
			xpaper-sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FRTRIANGLE:
		sizey=size*.866;
		sizex=size/2.0;
		FilledTriSet(w,xpaper+size,ypaper,
			xpaper-sizex,ypaper+sizey,
			xpaper-sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_LTRIANGLE:
		sizey=size*.866;
		sizex=size/2.0;
		TriSet(w,xpaper-size,ypaper,
			xpaper+sizex,ypaper+sizey,
			xpaper+sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FLTRIANGLE:
		sizey=size*.866;
		sizex=size/2.0;
		FilledTriSet(w,xpaper-size,ypaper,
			xpaper+sizex,ypaper+sizey,
			xpaper+sizex,ypaper-sizey,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_DIAMOND:
		QuadSet(w,xpaper,ypaper-size,
			xpaper+size,ypaper,
			xpaper,ypaper+size,
			xpaper-size,ypaper,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FDIAMOND:
		FilledQuadSet(w,xpaper,ypaper-size,
			xpaper+size,ypaper,
			xpaper,ypaper+size,
			xpaper-size,ypaper,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_HOURGLASS:
		QuadSet(w,xpaper-size,ypaper-size,
			xpaper+size,ypaper-size,
			xpaper-size,ypaper+size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FHOURGLASS:
		FilledQuadSet(w,xpaper-size,ypaper-size,
			xpaper+size,ypaper-size,
			xpaper-size,ypaper+size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_BOWTIE:
		QuadSet(w,xpaper-size,ypaper-size,
			xpaper-size,ypaper+size,
			xpaper+size,ypaper-size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;
	case XtMARKER_FBOWTIE:
		FilledQuadSet(w,xpaper-size,ypaper-size,
			xpaper-size,ypaper+size,
			xpaper+size,ypaper-size,
			xpaper+size,ypaper+size,
			color,XtLINE_SOLID);
		break;

	default:
		break;
	}
}

static void
DrawLegend(w)
SciPlotWidget w;
{
real x,y,len,height,height2,len2,ascent;
int i;
SciPlotList *p;

	if (w->plot.ShowLegend) {
		x=w->plot.x.LegendPos;
		y=w->plot.y.LegendPos;
		len=(real)w->plot.LegendLineSize;
		len2=len/2.0;
		height=FontnumHeight(w,w->plot.axisFont);
		height2=height/2.0;
		ascent=FontnumAscent(w,w->plot.axisFont);
		RectSet(w,x,y,
			x+w->plot.x.LegendSize-1.0-(real)w->plot.Margin,
			y+w->plot.y.LegendSize-1.0,
			w->plot.ForegroundColor,XtLINE_SOLID);
		x+=(real)w->plot.LegendMargin;
		y+=(real)w->plot.LegendMargin;
		
		for (i=0; i<w->plot.num_plotlist; i++) {
			p=w->plot.plotlist+i;
			if (p->draw) {
				LineSet(w,x,y+height2,x+len,y+height2,
					p->LineColor,p->LineStyle);
				DrawMarker(w,x+len2,y+height2,(real)3.0,
					p->PointColor,p->PointStyle);
				TextSet(w,x+len+(real)w->plot.LegendMargin,
					y+ascent,
					p->legend,w->plot.ForegroundColor,
					w->plot.axisFont);
				y+=height;
			}
		}
	}
}

static void
DrawCartesianAxes(w)
SciPlotWidget w;
{
double x,y,x1,y1,x2,y2,tic,val,height,majorval;
int j,precision;
char numberformat[16],label[512],tlabel[512];

	height=FontnumHeight(w,w->plot.labelFont);
	
	/* The w->plot.x.DrawOrigin and so on are in user data units.
	 * the x1, y1, x2, y2 are in pixels 
	 */
	x1=PlotX(w,w->plot.x.DrawOrigin);
	y1=PlotY(w,w->plot.y.DrawOrigin);
	x2=PlotX(w,w->plot.x.DrawMax);
	y2=PlotY(w,w->plot.y.DrawMax);

	LineSet(w,x1,y1,x2,y1,w->plot.ForegroundColor,XtLINE_SOLID);
	LineSet(w,x1,y1,x1,y2,w->plot.ForegroundColor,XtLINE_SOLID);

	precision=w->plot.x.Precision;
	if (w->plot.XLog) {
		val=w->plot.x.DrawOrigin;
		if (precision>0) precision--;	
	}
	else {
		val=w->plot.x.DrawOrigin;
	}
	x=PlotX(w,val);
	if (w->plot.DrawMajorTics)
		LineSet(w,x,y1+5,x,y1-5,w->plot.ForegroundColor,XtLINE_SOLID);

	get_number_format( numberformat, 16, precision, val*w->plot.x.Scalefact );
	if( w->plot.XFmtCallback != NULL )
		(*(w->plot.XFmtCallback))((Widget)w,val,label,512);
	else
		snprintf(label,512,numberformat,val*w->plot.x.Scalefact);
	TextSet(w,x,w->plot.y.AxisPos,label,w->plot.ForegroundColor,
		w->plot.axisFont);
	
	majorval=val;
	while ((majorval*1.00000001)<w->plot.x.DrawMax) {
		if (w->plot.XLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
			if (majorval*1.1>w->plot.x.DrawMax) break;
			tic=majorval;
			if (w->plot.DrawMinor||w->plot.DrawMinorTics) {
				for (j=2; j<w->plot.x.MinorNum; j++) {
					val=tic*(real)j;
					x=PlotX(w,val);
					if (w->plot.DrawMinor)
						LineSet(w,x,y1,x,y2,
							w->plot.ForegroundColor,
							XtLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w,x,y1,x,y1-3,
							w->plot.ForegroundColor,
							XtLINE_SOLID);
				}
			}
			val=tic*(real)w->plot.x.MinorNum;
			if (precision>0) precision--;	
		}
		else {
			tic=majorval;
			if (w->plot.DrawMinor||w->plot.DrawMinorTics) {
				for (j=1; j<w->plot.x.MinorNum; j++) {
					val=tic+w->plot.x.MajorInc*(real)j/
						w->plot.x.MinorNum;
					x=PlotX(w,val);
					if (w->plot.DrawMinor)
						LineSet(w,x,y1,x,y2,
							w->plot.ForegroundColor,
							XtLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w,x,y1,x,y1-3,
							w->plot.ForegroundColor,
							XtLINE_SOLID);
				}
			}
			val=tic+w->plot.x.MajorInc;
		}
		x=PlotX(w,val);
		if (w->plot.DrawMajor)
			LineSet(w,x,y1,x,y2,w->plot.ForegroundColor,
				XtLINE_DOTTED);
		if (w->plot.DrawMajorTics)
			LineSet(w,x,y1+5,x,y1-5,w->plot.ForegroundColor,
				XtLINE_SOLID);
		get_number_format( numberformat, 16, precision, val*w->plot.x.Scalefact );
		if( w->plot.XFmtCallback != NULL )
			(*(w->plot.XFmtCallback))((Widget)w,val,label,512);
		else
			snprintf(label,512,numberformat,val*w->plot.x.Scalefact);
		TextSet(w,x,w->plot.y.AxisPos,label,w->plot.ForegroundColor,
			w->plot.axisFont);
		majorval=val;
	}

	precision=w->plot.y.Precision;
	if (w->plot.YLog) {
		val=w->plot.y.DrawOrigin;
		if (precision>0) precision--;
	}
	else {
		val=w->plot.y.DrawOrigin;
	}
	y=PlotY(w,val);
	if (w->plot.DrawMajorTics)
		LineSet(w,x1+5,y,x1-5,y,w->plot.ForegroundColor,XtLINE_SOLID);

	get_number_format( numberformat, 16, precision, val*w->plot.y.Scalefact );
	snprintf(label,512,numberformat,val*w->plot.y.Scalefact);
	VTextSet(w,w->plot.x.AxisPos,y,label,w->plot.ForegroundColor,
		w->plot.axisFont);
	
	majorval=val;

/* majorval*1.0001 is a fudge to get rid of rounding errors that seem to
** occur when continuing to add the major axis increment. */
	while ((majorval*1.000001)<w->plot.y.DrawMax) {
		if (w->plot.YLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
			if (majorval*1.1>w->plot.y.DrawMax) break;
			tic=majorval;
			if (w->plot.DrawMinor||w->plot.DrawMinorTics) {
				for (j=2; j<w->plot.y.MinorNum; j++) {
					val=tic*(real)j;
					y=PlotY(w,val);
					if (w->plot.DrawMinor)
						LineSet(w,x1,y,x2,y,
							w->plot.ForegroundColor,
							XtLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w,x1,y,x1+3,y,
							w->plot.ForegroundColor,
							XtLINE_SOLID);
				}
			}
			val=tic*(real)w->plot.y.MinorNum;
			if (precision>0) precision--;	
		}
		else {
			tic=majorval;
			if (w->plot.DrawMinor||w->plot.DrawMinorTics) {
				for (j=1; j<w->plot.y.MinorNum; j++) {
					val=tic+w->plot.y.MajorInc*(real)j/
						w->plot.y.MinorNum;
					y=PlotY(w,val);
					if (w->plot.DrawMinor)
						LineSet(w,x1,y,x2,y,
							w->plot.ForegroundColor,
							XtLINE_WIDEDOT);
					if (w->plot.DrawMinorTics)
						LineSet(w,x1,y,x1+3,y,
							w->plot.ForegroundColor,
							XtLINE_SOLID);
				}
			}
			val=tic+w->plot.y.MajorInc;
		}
		y=PlotY(w,val);
		if (w->plot.DrawMajor)
			LineSet(w,x1,y,x2,y,w->plot.ForegroundColor,
				XtLINE_DOTTED);
		if (w->plot.DrawMajorTics)
			LineSet(w,x1-5,y,x1+5,y,w->plot.ForegroundColor,
				XtLINE_SOLID);
		get_number_format( numberformat, 16, precision, val*w->plot.y.Scalefact );
		snprintf(label,512,numberformat,val*w->plot.y.Scalefact);
		VTextSet(w,w->plot.x.AxisPos,y,label,w->plot.ForegroundColor,
			w->plot.axisFont);
		majorval=val;
	}
	if (w->plot.ShowTitle)
		TextSet(w,w->plot.x.TitlePos,w->plot.y.TitlePos,
			w->plot.plotTitle,w->plot.ForegroundColor,
			w->plot.titleFont);
	if (w->plot.ShowXLabel)
		TextCenter(w,w->plot.x.Origin+(w->plot.x.Size/2.0),
			w->plot.y.LabelPos,w->plot.xlabel,
			w->plot.ForegroundColor,w->plot.labelFont);
	if (w->plot.ShowYLabel) {
		if( w->plot.y.Scale_expon == 0. ) 
			snprintf( tlabel, 512, "%s", w->plot.ylabel );
		else
			snprintf( tlabel, 512, "%s/10**%d", w->plot.ylabel, -(w->plot.y.Scale_expon) );
		VTextCenter(w,w->plot.x.LabelPos,
			w->plot.y.Origin+(w->plot.y.Size/2.0),
			tlabel,w->plot.ForegroundColor,
			w->plot.labelFont);
		}
}

static void
DrawCartesianPlot(w)
SciPlotWidget w;
{
int i,j,jstart;
SciPlotList *p;

	ClipSet(w);
	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
		real x1,y1,x2,y2;
		
			jstart=1;
			if (((w->plot.XLog&&(p->data[0].x<=0.0))||
			      (w->plot.YLog&&(p->data[0].y<=0.0)))) {
				while ((jstart<p->number)&&
				       (((w->plot.XLog&&(p->data[jstart].x<=0.0))||
				        (w->plot.YLog&&(p->data[jstart].y<=0.0)))))
					jstart++;
				if (jstart<p->number) {
					x1=PlotX(w,p->data[jstart].x);
					y1=PlotY(w,p->data[jstart].y);
				}
			}
			else {
				x1=PlotX(w,p->data[0].x);
				y1=PlotY(w,p->data[0].y);
			}
			for (j=jstart; j<p->number; j++) {
				if (!((w->plot.XLog&&(p->data[j].x<=0.0))||
				      (w->plot.YLog&&(p->data[j].y<=0.0)))) {
					x2=PlotX(w,p->data[j].x);
					y2=PlotY(w,p->data[j].y);
					LineSet(w,x1,y1,x2,y2,
						p->LineColor,p->LineStyle);
					x1=x2;
					y1=y2;
				}
			}
		}
	}
	ClipClear(w);
	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
		real x2,y2;
	
			for (j=0; j<p->number; j++) {
				if (!((w->plot.XLog&&(p->data[j].x<=0.0))||
				      (w->plot.YLog&&(p->data[j].y<=0.0)))) {
					x2=PlotX(w,p->data[j].x);
					y2=PlotY(w,p->data[j].y);
					if ((x2>=w->plot.x.Origin)&&
					    (x2<=w->plot.x.Origin+w->plot.x.Size)&&
					    (y2>=w->plot.y.Origin)&&
					    (y2<=w->plot.y.Origin+w->plot.y.Size)) {
						DrawMarker(w,x2,y2,
							(real)3.0,
							p->PointColor,
							p->PointStyle);
						}
				}
			}
		}
	}
}

static void
DrawPolarAxes(w)
SciPlotWidget w;
{
real x1,y1,x2,y2,max,tic,val,height;
int i,j;
char numberformat[160],label[16];

	height=FontnumHeight(w,w->plot.labelFont);
	max=w->plot.PolarScale;
	PlotRTDegrees(w,0.0,0.0,&x1,&y1);
	PlotRTDegrees(w,max,0.0,&x2,&y2);
	LineSet(w,x1,y1,x2,y2,1,XtLINE_SOLID);
	for (i=45; i<360; i+=45) {
		PlotRTDegrees(w,max,(real)i,&x2,&y2);
		LineSet(w,x1,y1,x2,y2,w->plot.ForegroundColor,XtLINE_DOTTED);
	}
	for (i=1; i<=w->plot.x.MajorNum; i++) {
		tic=w->plot.PolarScale*
			(real)i/(real)w->plot.x.MajorNum;
		if (w->plot.DrawMinor||w->plot.DrawMinorTics) {
			for (j=1; j<w->plot.x.MinorNum; j++) {
				val=tic-w->plot.x.MajorInc*(real)j/
					w->plot.x.MinorNum;
				PlotRTDegrees(w,val,0.0,&x2,&y2);
				if (w->plot.DrawMinor)
					CircleSet(w,x1,y1,x2-x1,
						w->plot.ForegroundColor,XtLINE_WIDEDOT);
				if (w->plot.DrawMinorTics)
					LineSet(w,x2,y2-2.5,x2,y2+2.5,
						w->plot.ForegroundColor,XtLINE_SOLID);
			}
		}
		PlotRTDegrees(w,tic,0.0,&x2,&y2);
		if (w->plot.DrawMajor)
			CircleSet(w,x1,y1,x2-x1,w->plot.ForegroundColor,XtLINE_DOTTED);
		if (w->plot.DrawMajorTics)
			LineSet(w,x2,y2-5.0,x2,y2+5.0,w->plot.ForegroundColor,XtLINE_SOLID);
		get_number_format( numberformat, 160, w->plot.x.Precision, tic );
		snprintf(label,16,numberformat,tic);
		TextSet(w,x2,y2+height,label,w->plot.ForegroundColor,w->plot.axisFont);
	}
	if (w->plot.ShowTitle)
		TextSet(w,w->plot.x.TitlePos,w->plot.y.TitlePos,
			w->plot.plotTitle,w->plot.ForegroundColor,w->plot.titleFont);
}

static void
DrawPolarPlot(w)
SciPlotWidget w;
{
int i,j;
SciPlotList *p;

	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
		real x1,y1,x2,y2;
		
			PlotRT(w,p->data[0].x,p->data[0].y,&x1,&y1);
			DrawMarker(w,x1,y1,(real)3.0,
				p->PointColor,p->PointStyle);
			for (j=1; j<p->number; j++) {
				PlotRT(w,p->data[j].x,p->data[j].y,&x2,&y2);
				LineSet(w,x1,y1,x2,y2,
					p->LineColor,p->LineStyle);
				DrawMarker(w,x2,y2,(real)3.0,
					p->PointColor,p->PointStyle);
				x1=x2;
				y1=y2;
			}
		}
	}
}

static void
DrawAll(w)
SciPlotWidget w;
{
	if (w->plot.ChartType==XtCARTESIAN) {
		DrawCartesianAxes(w);
		DrawLegend(w);
		DrawCartesianPlot(w);
	}
	else {
		DrawPolarAxes(w);
		DrawLegend(w);
		DrawPolarPlot(w);
	}
}


/*
** Public Plot functions -------------------------------------------------
**
*/

real
SciPlotScreenToDataX(wi,xscreen)
Widget wi;
int xscreen;
{
	real xdata;
	double t;

	SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return(0.0);

	w=(SciPlotWidget)wi;

	t = (float)w->plot.x.DrawSize/(float)w->plot.x.Size*(
		(float)xscreen - (float)w->plot.x.Origin);

	if (w->plot.XLog) {
		t = pow(10.0,t);
		xdata = (float)w->plot.x.DrawOrigin * t;
		}
	else
		xdata = (float)w->plot.x.DrawOrigin + t;

	return xdata;
}

real
SciPlotScreenToDataY(wi,yscreen)
Widget wi;
int yscreen;
{
	real ydata;
	SciPlotWidget w;
	double t;

	if (!XtIsSciPlot(wi)) return(0.0);

	w=(SciPlotWidget)wi;

	t = -(float)w->plot.y.DrawSize/(float)w->plot.y.Size*(
		(float)yscreen - (float)w->plot.y.Origin - (float)w->plot.y.Size);

	if (w->plot.YLog) {
		t = pow(10.0,t);
		ydata = w->plot.y.DrawOrigin * t;
		}
	else
		ydata = w->plot.y.DrawOrigin + t;

	return ydata;
}

int
SciPlotAllocNamedColor(wi,name)
Widget wi;
char *name;
{
XColor used,exact;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return -1;

	w=(SciPlotWidget)wi;

	if (!XAllocNamedColor(XtDisplay(w),w->plot.cmap,name,&used,&exact))
		return 1;
	return ColorStore(w,used.pixel);
}

int
SciPlotAllocRGBColor(wi,r,g,b)
Widget wi;
int r,g,b;
{
XColor used;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return -1;

	w=(SciPlotWidget)wi;

	used.pixel=0;
	r*=256;
	g*=256;
	b*=256;
	if (r>65535) r=65535;
	if (g>65535) g=65535;
	if (b>65535) b=65535;
	used.red=(unsigned short)r;
	used.green=(unsigned short)g;
	used.blue=(unsigned short)b;
	if (!XAllocColor(XtDisplay(w),w->plot.cmap,&used)) return 1;
	return ColorStore(w,used.pixel);
}

void
SciPlotSetBackgroundColor(wi,color)
Widget wi;
int color;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	if (color<w->plot.num_colors) {
		w->plot.BackgroundColor=color;
		w->core.background_pixel=w->plot.colors[color];
	}
}

void
SciPlotSetForegroundColor(wi,color)
Widget wi;
int color;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	if (color<w->plot.num_colors) w->plot.ForegroundColor=color;
}

int
SciPlotListCreateFromData(wi,num,xlist,ylist,legend,pcolor,pstyle,lcolor,lstyle)
Widget wi;
int num;
real *xlist,*ylist;
char *legend;
int pstyle,pcolor,lstyle,lcolor;
{
int id;
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return -1;

	w=(SciPlotWidget)wi;

	id=List_New(w);
	p=w->plot.plotlist+id;
	List_SetReal(p,num,xlist,ylist);
	List_SetLegend(p,legend);
	List_SetStyle(p,pcolor,pstyle,lcolor,lstyle);
	return id;
}

int
SciPlotListCreateFromFloat(wi,num,xlist,ylist,legend)
Widget wi;
int num;
float *xlist,*ylist;
char *legend;
{
int id;
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return -1;

	w=(SciPlotWidget)wi;

	id=List_New(w);
	p=w->plot.plotlist+id;
	List_SetFloat(p,num,xlist,ylist);
	List_SetLegend(p,legend);
	List_SetStyle(p,1,XtMARKER_CIRCLE,1,XtLINE_SOLID);
	return id;
}

void
SciPlotListDelete(wi,id)
Widget wi;
int id;
{
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	p=List_Find(w,id);
	if (p) List_Delete(p);
}

void
SciPlotListUpdateFromFloat(wi,id,num,xlist,ylist)
Widget wi;
int id,num;
float *xlist,*ylist;
{
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	p=List_Find(w,id);
	if (p) List_SetFloat(p,num,xlist,ylist);
}

int
SciPlotListCreateFromDouble(wi,num,xlist,ylist,legend)
Widget wi;
int num;
double *xlist,*ylist;
char *legend;
{
int id;
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return -1;

	w=(SciPlotWidget)wi;

	id=List_New(w);
	p=w->plot.plotlist+id;
	List_SetDouble(p,num,xlist,ylist);
	List_SetLegend(p,legend);
	List_SetStyle(p,1,XtMARKER_CIRCLE,1,XtLINE_SOLID);
	return id;
}

void
SciPlotListUpdateFromDouble(wi,id,num,xlist,ylist)
Widget wi;
int id,num;
double *xlist,*ylist;
{
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	p=List_Find(w,id);
	if (p) List_SetDouble(p,num,xlist,ylist);
}

void
SciPlotListSetStyle(wi,id,pcolor,pstyle,lcolor,lstyle)
Widget wi;
int id;
int pstyle,pcolor,lstyle,lcolor;
{
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	p=List_Find(w,id);
	if (p) List_SetStyle(p,pcolor,pstyle,lcolor,lstyle);
}

void
SciPlotSetXAutoScale(wi)
Widget wi;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	w->plot.XAutoScale=True;
}

void
SciPlotSetXUserScale(wi,min,max)
Widget wi;
float min,max;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	if (min<max) {
		w->plot.XAutoScale=False;
		w->plot.UserMin.x=(real)min;
		w->plot.UserMax.x=(real)max;
	}
}

void
SciPlotSetYAutoScale(wi)
Widget wi;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	w->plot.YAutoScale=True;
}

void
SciPlotSetYUserScale(wi,min,max)
Widget wi;
float min,max;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	if (min<max) {
		w->plot.YAutoScale=False;
		w->plot.UserMin.y=(real)min;
		w->plot.UserMax.y=(real)max;
	}
}

void
SciPlotQueryXScale(wi,min,max)
Widget wi;
float *min,*max;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	*min = (float)(w->plot.x.DrawOrigin);
	*max = (float)(w->plot.x.DrawMax);
}

void
SciPlotQueryYScale(wi,min,max)
Widget wi;
float *min,*max;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	*min = (float)(w->plot.y.DrawOrigin);
	*max = (float)(w->plot.y.DrawMax);
}

void
SciPlotPrintStatistics(wi)
Widget wi;
{
int i,j;
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	printf("Title=%s\nxlabel=%s\tylabel=%s\n",
		w->plot.plotTitle,w->plot.xlabel,w->plot.ylabel);
	printf("ChartType=%d\n",w->plot.ChartType);
	printf("Degrees=%d\n",w->plot.Degrees);
	printf("XLog=%d\tYLog=%d\n",w->plot.XLog,w->plot.YLog);
	printf("XAutoScale=%d\tYAutoScale=%d\n",
		w->plot.XAutoScale,w->plot.YAutoScale);
	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
			printf("\nLegend=%s\n",p->legend);
			printf("Styles: point=%d line=%d  Color: point=%d line=%d\n",
				p->PointStyle,p->LineStyle,p->PointColor,p->LineColor);
			for (j=0; j<p->number; j++)
				printf("%f\t%f\n",p->data[j].x,p->data[j].y);
			printf("\n");
		}
	}
}

void
SciPlotExportData(wi,fd)
Widget wi;
FILE *fd;
{
int i,j;
SciPlotList *p;
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;

	fprintf(fd,"Title=%s\n\n",w->plot.plotTitle);
	for (i=0; i<w->plot.num_plotlist; i++) {
		p=w->plot.plotlist+i;
		if (p->draw) {
			fprintf(fd,"Legend=%s\n%s\t%s\n",
				p->legend,w->plot.xlabel,w->plot.ylabel);
			for (j=0; j<p->number; j++)
				fprintf(fd,"%f\t%f\n",p->data[j].x,p->data[j].y);
			fprintf(fd,"\n");
		}
	}
}

void
SciPlotAddXAxisCallback(wi,cb)
Widget wi;
void (*cb)(Widget wi, float val, char *s, size_t slen);
{
SciPlotWidget w;
	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	w->plot.XFmtCallback = cb;
}

void
SciPlotUpdate(wi)
Widget wi;
{
SciPlotWidget w;

	if (!XtIsSciPlot(wi)) return;

	w=(SciPlotWidget)wi;
	EraseAll(w);
#ifdef DEBUG_SCIPLOT
	SciPlotPrintStatistics(w);
#endif
	ComputeAll(w);
	DrawAll(w);
}

