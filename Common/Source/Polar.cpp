/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

   $Id$
*/

#include "StdAfx.h"
#ifndef __MINGW32__
#if defined(CECORE)
#include "winbase.h"
#endif
#if (WINDOWSPC<1)
#include "projects.h"
#endif
#else
#include "wcecompat/ts_string.h"
#endif

#include "options.h"
#include "Defines.h"
#include "externs.h"
#include "lk8000.h"
#include "McReady.h"


bool ReadWinPilotPolarInternal(int i);


// This is calculating the weight difference for the chosen wingloading
// Remember that polar files have a weight indicated that includes the pilot..
// Check that WingArea is NOT zero! Winpilot polars had no WingArea configurable!

void WeightOffset(double wload) {
  double calcweight;

  if (GlidePolar::WingArea<1) { // 100131
	// we should lock calculation thread probably
	if (GlidePolar::WeightOffset!=0) GlidePolar::WeightOffset=0;
	return;
  }

  // WEIGHTS[2] is full ballast
  // BALLAST is percentage of full ballast
  // new weight = (wingload * wingarea) - ballast
  calcweight=(wload*GlidePolar::WingArea) - (WEIGHTS[2]*BALLAST);
  GlidePolar::WeightOffset = calcweight-(WEIGHTS[0]+WEIGHTS[1]);

  GlidePolar::SetBallast(); // BUGFIX 101002
}


void PolarWinPilot2XCSoar(double POLARV[3], double POLARW[3], double ww[2]) {
  double d;
  double v1,v2,v3;
  double w1,w2,w3;

  v1 = POLARV[0]/3.6; v2 = POLARV[1]/3.6; v3 = POLARV[2]/3.6;
  //	w1 = -POLARV[0]/POLARLD[0];
  //    w2 = -POLARV[1]/POLARLD[1];
  //    w3 = -POLARV[2]/POLARLD[2];
  w1 = POLARW[0]; w2 = POLARW[1]; w3 = POLARW[2];

  d = v1*v1*(v2-v3)+v2*v2*(v3-v1)+v3*v3*(v1-v2);
  if (d == 0.0)
    {
      POLAR[0]=0;
    }
  else
    {
      POLAR[0]=((v2-v3)*(w1-w3)+(v3-v1)*(w2-w3))/d;
    }
  d = v2-v3;
  if (d == 0.0)
    {
      POLAR[1]=0;
    }
  else
    {
      POLAR[1] = (w2-w3-POLAR[0]*(v2*v2-v3*v3))/d;
    }

  // these 0 and 1 are always used as a single weight: always 0+1 everywhere
  // If WEIGHT 0 is used also WEIGHT 1 is used together, so it is unnecessary to keep both values.
  // however it doesnt hurt .
  // For this reason, the 70kg pilot weight is not important.
  // If we want to adjust wingloading, we just need to change gross weight.
  WEIGHTS[0] = 70;                      // Pilot weight 
  WEIGHTS[1] = ww[0]-WEIGHTS[0];        // Glider empty weight
  WEIGHTS[2] = ww[1];                   // Ballast weight

  POLAR[2] = (double)(w3 - POLAR[0] *v3*v3 - POLAR[1]*v3);

  // now scale off weight
  POLAR[0] = POLAR[0] * (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);
  POLAR[2] = POLAR[2] / (double)sqrt(WEIGHTS[0] + WEIGHTS[1]);

}


bool ReadWinPilotPolar(void) {

  TCHAR	szFile[MAX_PATH] = TEXT("\0");
  TCHAR ctemp[80];
  TCHAR TempString[READLINE_LENGTH+1];
  HANDLE hFile;

  double POLARV[3];
  double POLARW[3];
  double ww[2];
  bool foundline = false;

#ifdef HAVEEXCEPTIONS
  __try{
#endif

    // LS3 values, overwritten by loaded values
    // *LS-3  WinPilot POLAR file: MassDryGross[kg], MaxWaterBallast[liters], Speed1[km/h], Sink1[m/s], Speed2, Sink2, Speed3, Sink3
    // 403, 101, 115.03, -0.86, 174.04, -1.76, 212.72,	-3.4
    ww[0]= 403.0; // 383
    ww[1]= 101.0; // 121
    POLARV[0]= 115.03;
    POLARW[0]= -0.86;
    POLARV[1]= 174.04;
    POLARW[1]= -1.76;
    POLARV[2]= 212.72;
    POLARW[2]= -3.4;

    GetRegistryString(szRegistryPolarFile, szFile, MAX_PATH);
    if (_tcscmp(szFile,_T(""))==0) {
	StartupStore(_T("... Empty polar file, using Default%s"),NEWLINE);
	wcscpy(szFile,_T("%LOCAL_PATH%\\\\_Polars\\Default.plr"));
    }

    ExpandLocalPath(szFile);
    StartupStore(_T(". Loading polar file <%s>%s"),szFile,NEWLINE);

    hFile = CreateFile(szFile,GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);

    if (hFile != INVALID_HANDLE_VALUE ){
#ifdef HAVEEXCEPTIONS
      __try{
#endif

        while(ReadString(hFile,READLINE_LENGTH,TempString) && (!foundline)){

		if (_tcslen(TempString) <10) continue;

          if(_tcsstr(TempString,TEXT("*")) != TempString) // Look For Comment
            {
              PExtractParameter(TempString, ctemp, 0);
		// weight of glider + pilot
              ww[0] = StrToDouble(ctemp,NULL);
		// weight of loadable ballast
              PExtractParameter(TempString, ctemp, 1);
              ww[1] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 2);
              POLARV[0] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 3);
              POLARW[0] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 4);
              POLARV[1] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 5);
              POLARW[1] = StrToDouble(ctemp,NULL);

              PExtractParameter(TempString, ctemp, 6);
              POLARV[2] = StrToDouble(ctemp,NULL);
              PExtractParameter(TempString, ctemp, 7);
              POLARW[2] = StrToDouble(ctemp,NULL);

		_stprintf(ctemp,_T(""));
              	PExtractParameter(TempString, ctemp, 8);
		if ( _tcscmp(ctemp,_T("")) != 0) {
              		GlidePolar::WingArea = StrToDouble(ctemp,NULL);
			// StartupStore(_T(". Polar file has wing area=%f%s"),GlidePolar::WingArea,NEWLINE);
		} else {
	      		GlidePolar::WingArea = 0.0;
			StartupStore(_T("... WARNING Polar file has NO wing area%s"),NEWLINE);
		}
		
              PolarWinPilot2XCSoar(POLARV, POLARW, ww);

              foundline = true;
            }
        }

	int i;

	// Reset flaps values after loading a new polar, and init FlapsPos for the first time
	for (i=0; i<MAX_FLAPS; i++) {
		GlidePolar::FlapsPos[i]=0.0;
		wcscpy(GlidePolar::FlapsName[i],_T("???"));
	}
	GlidePolar::FlapsPosCount=0;
	GlidePolar::FlapsMass=0.0;

	// Unless we check valid string, even with empty string currentFlapsPos will be positive,
	// and thus force Flaps calculations even with no extended polar.
	// Let's allow empty lines and comments in the polar file, before the flaps line is found.
	// 
	do {
	   if (_tcslen(TempString) <10) continue;
	   if(_tcsstr(TempString,TEXT("*")) == TempString) continue;
    	   // try to read flaps configuration line
     	   PExtractParameter(TempString, ctemp, 0);
     	   GlidePolar::FlapsMass = StrToDouble(ctemp,NULL);
     	   PExtractParameter(TempString, ctemp, 1);
     	   int flapsCount = (int) StrToDouble(ctemp,NULL);

     	   // int currentFlapsPos = 0;
     	   // GlidePolar::FlapsPos[currentFlapsPos][0] = 0.0;  // no need, already initialised

     	   int currentFlapsPos=1;
     	   for (i=2; i <= flapsCount*2; i=i+2) {
	        PExtractParameter(TempString, ctemp, i);
	        GlidePolar::FlapsPos[currentFlapsPos] = StrToDouble(ctemp,NULL);				
			if (GlidePolar::FlapsPos[currentFlapsPos] > 0) {
			  GlidePolar::FlapsPos[currentFlapsPos] = GlidePolar::FlapsPos[currentFlapsPos]/TOKPH;
			}
	        PExtractParameter(TempString, ctemp, i+1);
		ctemp[MAXFLAPSNAME]='\0';
		if (ctemp[_tcslen(ctemp)-1]=='\r' || ctemp[_tcslen(ctemp)-1]=='\n')
			ctemp[_tcslen(ctemp)-1]='\0'; // remove trailing cr
		wcscpy(GlidePolar::FlapsName[currentFlapsPos],ctemp);
		if (currentFlapsPos >= (MAX_FLAPS-1)) break; // safe check
	        currentFlapsPos++;
       	   }
	   wcscpy(GlidePolar::FlapsName[0],GlidePolar::FlapsName[1]);
           GlidePolar::FlapsPos[currentFlapsPos] = MAXSPEED;
           wcscpy(GlidePolar::FlapsName[currentFlapsPos],ctemp);
           currentFlapsPos++;
           GlidePolar::FlapsPosCount = currentFlapsPos; 
	   break;
	} while(ReadString(hFile,READLINE_LENGTH,TempString));

        // file was OK, so save it
        if (foundline) {
          ContractLocalPath(szFile);
          SetRegistryString(szRegistryPolarFile, szFile);
        }
#ifdef HAVEEXCEPTIONS
      }__finally
#endif
      {
        CloseHandle (hFile);
      }
    } 
    else {
	StartupStore(_T("... Polar file <%s> not found!%s"),szFile,NEWLINE);
    }
#ifdef HAVEEXCEPTIONS
  }__except(EXCEPTION_EXECUTE_HANDLER){
    foundline = false;
  }
#endif
  return(foundline);

}



typedef double PolarCoefficients_t[3];
typedef double WeightCoefficients_t[3];


void CalculateNewPolarCoef(void)
{

  // StartupStore(TEXT(". Calculate New Polar Coef%s"),NEWLINE);

  GlidePolar::WeightOffset=0; // 100131 

  // Load polar file
  if (ReadWinPilotPolar()) return;

  // Polar File error, we load a Ka6 just to be safe

  POLAR[0]=-0.0538770500225782443497;
  POLAR[1]=0.1323114348;
  POLAR[2]=-0.1273364037098239098543;
  WEIGHTS[0]=70;
  WEIGHTS[1]=190;
  WEIGHTS[2]=1;
  GlidePolar::WingArea = 12.4;

  // Probably called from wrong thread - check
  MessageBoxX(NULL, 
              gettext(TEXT("_@M920_")), // Error loading Polar file!
              gettext(TEXT("_@M791_")), // Warning
              MB_OK|MB_ICONERROR);

}



typedef struct WinPilotPolarInternal {
  TCHAR name[50];
  double ww0;
  double ww1;
  double v0;
  double w0;
  double v1;
  double w1;
  double v2;
  double w2;
  double wing_area;
} WinPilotPolarInternal;


// REMEMBER: add new polars at the bottom, or old configuration will get a different polar value
// Also remember, currently 300 items limit in WindowControls.h DFE_  enums.
// THIS IS NOW UNUSED
WinPilotPolarInternal WinPilotPolars[] = 
{
  {TEXT("ERROR"), 670,100,100,-1.29,120,-1.61,150,-2.45,15.3},
  }; //   0-x

TCHAR* GetWinPilotPolarInternalName(int i) {
  if ( (unsigned) i >= (sizeof(WinPilotPolars)/sizeof(WinPilotPolarInternal)) ) {
    return NULL; // error
  }
  return WinPilotPolars[i].name;
}

bool ReadWinPilotPolarInternal(int i) {
  double POLARV[3];
  double POLARW[3];
  double ww[2];

  if (!(i < (int)(sizeof(WinPilotPolars) / sizeof(WinPilotPolars[0])))) { 
	return(FALSE);
  }

  // 0 is glider+pilot,  1 is ballast
  ww[0] = WinPilotPolars[i].ww0;
  ww[1] = WinPilotPolars[i].ww1;
  POLARV[0] = WinPilotPolars[i].v0;
  POLARV[1] = WinPilotPolars[i].v1;
  POLARV[2] = WinPilotPolars[i].v2;
  POLARW[0] = WinPilotPolars[i].w0;
  POLARW[1] = WinPilotPolars[i].w1;
  POLARW[2] = WinPilotPolars[i].w2;
  PolarWinPilot2XCSoar(POLARV, POLARW, ww);
  GlidePolar::WingArea = WinPilotPolars[i].wing_area;

  return(TRUE);

}








