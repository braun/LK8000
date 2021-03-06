/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "StdAfx.h"
#include <stdio.h>
#ifndef __MINGW32__
#if defined(CECORE)
#include "winbase.h"
#endif
#if (WINDOWSPC<1)
#include "projects.h"
#endif
#endif
#include "options.h"
#include "Defines.h"
#include "externs.h"
#include "lk8000.h"
#include "McReady.h"


bool InitLDRotary(ldrotary_s *buf) {
short i, bsize;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif

	switch (AverEffTime) {
		case ae15seconds:
			bsize=15;	// useless, LDinst already there
			break;
		case ae30seconds:
			bsize=30;	// limited useful
			break;
		case ae60seconds:
			bsize=60;	// starting to be valuable
			break;
		case ae90seconds:
			bsize=90;	// good interval
			break;
		case ae2minutes:
			bsize=120;	// other software's interval
			break;
		case ae3minutes:
			bsize=180;	// probably too long interval
			break;
		default:
			bsize=3; // make it evident 
			break;
	}
	//if (bsize <3 || bsize>MAXLDROTARYSIZE) return false;
	for (i=0; i<MAXLDROTARYSIZE; i++) {
		buf->distance[i]=0;
		buf->altitude[i]=0;
		buf->ias[i]=0;
	}
	buf->totaldistance=0;
	buf->totalias=0;
	buf->start=-1;
	buf->size=bsize;
	buf->valid=false;
#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"InitLdRotary size=%d\r\n",buf->size);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
  return false; 
}

void InsertLDRotary(ldrotary_s *buf, int distance, int altitude) {
static short errs=0;
#ifdef DEBUG_ROTARY
char ventabuffer[200];
FILE *fp;
#endif
	if (CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"OnGround, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	
	if (CALCULATED_INFO.Circling == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Circling, ignore LDrotary\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}


	//CALCULATED_INFO.Odometer += distance;
	if (distance<3 || distance>150) { // just ignore, no need to reset rotary
		if (errs>2) {
#ifdef DEBUG_ROTARY
			sprintf(ventabuffer,"Rotary reset after exceeding errors\r\n");
			if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
				    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
			InitLDRotary(&rotaryLD);
			errs=0;
			return;

		}
		errs++;
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"(errs=%d) IGNORE INVALID distance=%d altitude=%d\r\n",errs,distance,altitude);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		return;
	}
	errs=0;

	if (++buf->start >=buf->size) { 
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"*** rotary reset and VALID=TRUE ++bufstart=%d >=bufsize=%d\r\n",buf->start, buf->size);
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		buf->start=0;
		buf->valid=true; // flag for a full usable buffer 
	}
	// need to fill up buffer before starting to empty it
	if ( buf->valid == true) {
		buf->totaldistance-=buf->distance[buf->start];
		buf->totalias-=buf->ias[buf->start];
	}
	buf->totaldistance+=distance;
	buf->distance[buf->start]=distance;
	// insert IAS in the rotary buffer, either real or estimated
	if (GPS_INFO.AirspeedAvailable) {
                buf->totalias += (int)GPS_INFO.IndicatedAirspeed;
                buf->ias[buf->start] = (int)GPS_INFO.IndicatedAirspeed;
	} else {
                buf->totalias += (int)CALCULATED_INFO.IndicatedAirspeedEstimated;
                buf->ias[buf->start] = (int)CALCULATED_INFO.IndicatedAirspeedEstimated;
	}
	buf->altitude[buf->start]=altitude;
#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"insert buf[%d/%d], distance=%d totdist=%d\r\n",buf->start, buf->size-1, distance,buf->totaldistance);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
		    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
}

//#define DEBUG_EQMC 1
/*
 * returns 0 if invalid, 999 if too high
 * EqMc is negative when no value is available, because recalculated and buffer still not usable
 */
double CalculateLDRotary(ldrotary_s *buf, DERIVED_INFO *Calculated ) {

	int altdiff;
	double eff;
	short bcold;
#ifdef DEBUG_ROTARY
	char ventabuffer[200];
	FILE *fp;
#endif
	double averias;
	double avertas;

	if ( CALCULATED_INFO.Circling == TRUE || CALCULATED_INFO.OnGround == TRUE) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Not Calculating, on ground or circling\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		#if DEBUG_EQMC
		StartupStore(_T("... Circling or grounded, EqMc -1 (---)\n"));
		#endif
		Calculated->EqMc = -1;
		return(0);
	}

	if ( buf->start <0) {
#ifdef DEBUG_ROTARY
		sprintf(ventabuffer,"Calculate: invalid buf start<0\r\n");
		if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
			    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif
		#if DEBUG_EQMC
		StartupStore(_T("... Invalid buf start <0, EqMc -1 (---)\n"));
		#endif
		Calculated->EqMc = -1;
		return(0);
	}

	ldrotary_s bc;
  	memcpy(&bc, buf, sizeof(ldrotary_s));

	if (bc.valid == false ) {
		if (bc.start==0) {
			#if DEBUG_EQMC
			StartupStore(_T("... bc.valid is false, bc.start is 0, EqMc -1 (---)\n"));
			#endif
			Calculated->EqMc = -1;
			return(0); // unavailable
		}
		bcold=0;
	} else {

		if (bc.start < (bc.size-1))
			bcold=bc.start+1;
		else
			bcold=0;
	}

	altdiff= bc.altitude[bcold] - bc.altitude[bc.start];

	// if ( bc.valid == true ) {
	// bcsize<=0  should NOT happen, but we check it for safety
	if ( (bc.valid == true) && bc.size>0 ) {
		averias = bc.totalias/bc.size;
		// According to Welch & Irving, suggested by Dave..
		// MC = Vso*[ (V/Vo)^3 - (Vo/V)]
		// Vso: sink at best L/D
		// Vo : speed at best L/D
		// V  : TAS

		avertas=averias*AirDensityRatio(Calculated->NavAltitude);

		double dtmp= avertas/GlidePolar::Vbestld;

		Calculated->EqMc = -1*GlidePolar::sinkratecache[GlidePolar::Vbestld] * ( (dtmp*dtmp*dtmp) - ( GlidePolar::Vbestld/avertas));
		#if DEBUG_EQMC
		StartupStore(_T(".. eMC=%.2f (=%.1f)  Averias=%f Avertas=%f kmh, sinktas=%.1f ms  sinkmc0=%.1f ms Vbestld=%.1f\n"),
		Calculated->EqMc, Calculated->EqMc, averias*TOKPH, avertas*TOKPH,-1*GlidePolar::sinkratecache[(int)avertas], 
		GlidePolar::sinkratecache[GlidePolar::Vbestld], GlidePolar::Vbestld*TOKPH);
		#endif

	}
	#if DEBUG_EQMC
	else {
		StartupStore(_T(".... bc.valid=%d bc.size=%d <=0, no eqMc\n"), bc.valid,bc.start);
	}
	#endif

	if (altdiff == 0 ) {
		return(INVALID_GR); // infinitum
	}
	eff= (double)bc.totaldistance / (double)altdiff;

#ifdef DEBUG_ROTARY
	sprintf(ventabuffer,"bcstart=%d bcold=%d altnew=%d altold=%d altdiff=%d totaldistance=%d eff=%d\r\n",
		bc.start, bcold,
		bc.altitude[bc.start], bc.altitude[bcold], altdiff, bc.totaldistance, eff);
	if ((fp=fopen("DEBUG.TXT","a"))!= NULL)
                    {;fprintf(fp,"%s\n",ventabuffer);fclose(fp);}
#endif

	if (eff>MAXEFFICIENCYSHOW) eff=INVALID_GR;

	return(eff);

}

#if 0
// Example code used for rotary buffering
bool InitFilterBuffer(ifilter_s *buf, short bsize) {
short i;
	if (bsize <3 || bsize>RASIZE) return false;
	for (i=0; i<RASIZE; i++) buf->array[i]=0;
	buf->start=-1;
	buf->size=bsize;
	return true;
}

void InsertRotaryBuffer(ifilter_s *buf, int value) {
	if (++buf->start >=buf->size) { 
		buf->start=0;
	}
	buf->array[buf->start]=value;
}

int FilterFast(ifilter_s *buf, int minvalue, int maxvalue) {

  ifilter_s bc;
  memcpy(&bc, buf, sizeof(ifilter_s));

  short i,nc,iter;
  int s, *val;
  float aver=0.0, oldaver, low=minvalue, high=maxvalue, cutoff;

  for (iter=0; iter<MAXITERFILTER; iter++) {
 	 for (i=0, nc=0, s=0; i<bc.size; i++) {
		val=&bc.array[i];
		if (*val >=low && *val <=high) { s+=*val; nc++; }
	  }
	  if (nc==0) { aver=0.0; break; }
	  oldaver=aver; aver=((float)s/nc);
 	  //printf("Sum=%d count=%d Aver=%0.3f (old=%0.3f)\n",s,nc,aver,oldaver);
	  if (oldaver==aver) break;
	  cutoff=aver/50; // 2%
 	  low=aver-cutoff;
 	  high=aver+cutoff;
  }
  //printf("Found: aver=%d (%0.3f) after %d iterations\n",(int)aver, aver, iter);
  return ((int)aver);

}

int FilterRotary(ifilter_s *buf, int minvalue, int maxvalue) {

  ifilter_s bc;
  memcpy(&bc, buf, sizeof(ifilter_s));

  short i,curs,nc,iter;
  int s, val;
  float aver, low, high, cutoff;

  low  = minvalue;
  high = maxvalue;

  for (iter=0; iter<MAXITERFILTER; iter++) {
 	 for (i=0, nc=0, s=0,curs=bc.start; i<bc.size; i++) {

		val=bc.array[curs];
		if (val >=low && val <=high) {
			s+=val;
			nc++;
		}
		if (++curs >= bc.size ) curs=0;
	  }
	  if (nc==0) {
		aver=0.0;
		break;
	  }
	  aver=((float)s/nc);
 	  //printf("Sum=%d count=%d Aver=%0.3f\n",s,nc,aver);

	  cutoff=aver/50; // 2%
 	  low=aver-cutoff;
 	  high=aver+cutoff;
  }

  //printf("final: aver=%d\n",(int)aver);
  return ((int)aver);

}
#endif
/*
main(int argc, char *argv[])
{
  short i;
  ifilter_s buf;
  InitFilterBuffer(&buf,20);
  int values[20] = { 140,121,134,119,116,118,121,122,120,124,119,117,116,130,122,119,110,118,120,121 };
  for (i=0; i<20; i++) InsertRotaryBuffer(&buf, values[i]);
  FilterFast(&buf, 70,200 );
  buf.start=10; 
  for (i=0; i<20; i++) InsertRotaryBuffer(&buf, values[i]);
  FilterFast(&buf, 70,200 );
}
*/


