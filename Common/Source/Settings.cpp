/*
   LK8000 Tactical Flight Computer -  WWW.LK8000.IT
   Released under GNU/GPL License v.2
   See CREDITS.TXT file for authors and copyrights

  $Id$
*/


#include "StdAfx.h"
#include "lk8000.h"
#include "InputEvents.h"
#include "Terrain.h"
#include "InfoBoxLayout.h"
#include "Waypointparser.h"
#include "RasterTerrain.h"
#include "McReady.h"
#include "AirfieldDetails.h"



void SettingsEnter() {
  MenuActive = true;

  MapWindow::SuspendDrawingThread();
  // This prevents the map and calculation threads from doing anything
  // with shared data while it is being changed.

  MAPFILECHANGED = FALSE;
  AIRSPACEFILECHANGED = FALSE;
  AIRFIELDFILECHANGED = FALSE;
  WAYPOINTFILECHANGED = FALSE;
  TERRAINFILECHANGED = FALSE;
  TOPOLOGYFILECHANGED = FALSE;
  POLARFILECHANGED = FALSE;
  LANGUAGEFILECHANGED = FALSE;
  STATUSFILECHANGED = FALSE;
  INPUTFILECHANGED = FALSE;
  COMPORTCHANGED = FALSE;
}


void SettingsLeave() {
  if (!GlobalRunning) return; 

  SwitchToMapWindow();

  // Locking everything here prevents the calculation thread from running,
  // while shared data is potentially reloaded.
 
  LockFlightData();
  LockTaskData();

  MenuActive = false;

  // 101020 LKmaps contain only topology , so no need to force total reload!
  if(MAPFILECHANGED) {
	if (LKTopo==0) {
		AIRSPACEFILECHANGED = TRUE;
		AIRFIELDFILECHANGED = TRUE;
		WAYPOINTFILECHANGED = TRUE;
		TERRAINFILECHANGED  = TRUE;
	}
	TOPOLOGYFILECHANGED = TRUE;
  } 

  if (TERRAINFILECHANGED) {
	RasterTerrain::CloseTerrain();
	RasterTerrain::OpenTerrain();
	SetHome(WAYPOINTFILECHANGED==TRUE);
	RasterTerrain::ServiceFullReload(GPS_INFO.Latitude, GPS_INFO.Longitude);
	MapWindow::ForceVisibilityScan = true;
  }

  if((WAYPOINTFILECHANGED) || (AIRFIELDFILECHANGED)) {
	SaveDefaultTask(); //@ 101020 BUGFIX
	ClearTask();
	ReadWayPoints();
	StartupStore(_T(". Total %d waypoints%s"),NumberOfWayPoints,NEWLINE);
	InitWayPointCalc();
	ReadAirfieldFile();
	SetHome(true); // force home reload

	if (WAYPOINTFILECHANGED) {
		SaveRecentList();
		LoadRecentList();
		RangeLandableNumber=0;
		RangeAirportNumber=0;
		RangeTurnpointNumber=0;
		CommonNumber=0;
		SortedNumber=0;
		// SortedTurnpointNumber=0; 101222
		LastDoRangeWaypointListTime=0;
		LKForceDoCommon=true;
		LKForceDoNearest=true;
		LKForceDoRecent=true;
		// LKForceDoNearestTurnpoint=true; 101222
	}
	InputEvents::eventTaskLoad(_T(LKF_DEFAULTASK)); //@ BUGFIX 101020
  } 

  if (TOPOLOGYFILECHANGED) {
	CloseTopology();
	OpenTopology();
	MapWindow::ForceVisibilityScan = true;
  }
  
  if(AIRSPACEFILECHANGED) {
	CAirspaceManager::Instance().CloseAirspaces();
	CAirspaceManager::Instance().ReadAirspaces();
	CAirspaceManager::Instance().SortAirspaces();
	MapWindow::ForceVisibilityScan = true;
  }  
  
  if (POLARFILECHANGED) {
	CalculateNewPolarCoef();
	GlidePolar::SetBallast();
  }
  
  if (AIRFIELDFILECHANGED
      || AIRSPACEFILECHANGED
      || WAYPOINTFILECHANGED
      || TERRAINFILECHANGED
      || TOPOLOGYFILECHANGED
      ) {
	CloseProgressDialog();
	SetFocus(hWndMapWindow);
  }
  
  UnlockTaskData();
  UnlockFlightData();

  if(!SIMMODE && COMPORTCHANGED) {
      LKForceComPortReset=true;
      // RestartCommPorts(); 110605
  }

  MapWindow::ResumeDrawingThread();
  // allow map and calculations threads to continue on their merry way
}


void SystemConfiguration(void) {
  if (!SIMMODE) {
	if (LockSettingsInFlight && CALCULATED_INFO.Flying) {
		DoStatusMessage(TEXT("Settings locked in flight"));
		return;
	}
  }

  SettingsEnter();
  dlgConfigurationShowModal();
  SettingsLeave();
}

