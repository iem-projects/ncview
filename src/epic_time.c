/*
	This first part is taken directly from the PMEL EPIC library, which has the 
	following contact information attached:

	--------------------------------------------------------------------------
	Willa Zhu                           Phone:    206-526-6208
	NOAA/PMEL/OCRD                      FAX:      206-526-6744
	7600 Sand Point Way NE              OMNET:    TAO.PMEL
	Seattle, WA 98115                   Internet: willa@noaapmel.gov

*/

#include "ncview.includes.h"
#include "ncview.defines.h"
#include "ncview.protos.h"

#define JULGREG   2299161

void
ep_time_to_mdyhms(time, mon, day, yr, hour, min, sec)
     long *time;
     int *mon, *day, *yr, *hour, *min;
     float *sec;
{
/*
 * convert eps time format to mdy hms
 */
  long ja, jalpha, jb, jc, jd, je;

  while(time[1] >= 86400000) { /* increament days if ms larger then one day */
    time[0]++;
    time[1] -= 86400000;
  }

  if(time[0] >= JULGREG) {
    jalpha=(long)(((double) (time[0]-1867216)-0.25)/36524.25);
    ja=time[0]+1+jalpha-(long)(0.25*jalpha);
  } else
    ja=time[0];

  jb=ja+1524;
  jc=(long)(6680.0+((double)(jb-2439870)-122.1)/365.25);
  jd=(long)(365*jc+(0.25*jc));
  je=(long)((jb-jd)/30.6001);
  *day=jb-jd-(int)(30.6001*je);
  *mon=je-1;
  if(*mon > 12) *mon -= 12;
  *yr=jc-4715;
  if(*mon > 2) --(*yr);

  if(*yr <=0) --(*yr);

  ja = time[1]/1000;
  *hour = ja/3600;
  *min = (ja - (*hour)*3600)/60;
  *sec = (float)(time[1] - ((*hour)*3600 + (*min)*60)*1000)/1000.0f;
}

/*************************************************************************/
	int
epic_istime0( int fileid, NCVar *v, NCDim *d )
{
	if( (d->units != NULL) && strncmp( d->units, "True Julian Day", 15 ) == 0 ) {
		return( 1 );
		}
	
	return( 0 );
}


/*************************************************************************/
	int
epic_calc_tgran( int fileid, NCDim *d )
{
	/* EPIC is strange because it can use TWO dimvars for time.
	 * don't know how to handle the millisecond part yet, so
	 * just fake it by indicating day-like granularity.  Will
	 * have to be fixed at some point.
	 */
	return( TGRAN_DAY );	
}

/*************************************************************************/
	void
epic_fmt_time( char *temp_string, size_t temp_string_len, double new_dimval, NCDim *dim )
{
	long	epic_time[2];
	int	mon, day, yr, hour, min;
	float	sec;
	static  char months[12][4] = { 	"Jan\0", "Feb\0", "Mar\0", "Apr\0",
					"May\0", "Jun\0", "Jul\0", "Aug\0",
					"Sep\0", "Oct\0", "Nov\0", "Dec\0"};

	epic_time[0] = (long)new_dimval;
	epic_time[1] = 0L;
	ep_time_to_mdyhms(epic_time, &mon, &day, &yr, &hour, &min, &sec);

	snprintf( temp_string, temp_string_len, "%1d-%s-%04d %02d:%02d", day, months[mon-1],
		yr, hour, min );
	temp_string[ temp_string_len-1 ] = '\0';
}

