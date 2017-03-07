// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//
#ifndef STDAFX_HPP
#define STDAFX_HPP

#include <stdio.h>
#include <tchar.h>

// Classes for handling observations RINEX files (data)
#include "Rinex3ObsHeader.hpp"
#include "Rinex3ObsData.hpp"
#include "Rinex3ObsStream.hpp"
//
// Classes for handling Clock RINEX files (data)
#include "Rinex3ClockHeader.hpp"
#include "Rinex3ClockData.hpp"
#include "Rinex3ClockStream.hpp"

#include"IonoModelStore.hpp"

// Classes for handling satellite navigation parameters RINEX
// files (ephemerides)
#include "Rinex3NavHeader.hpp"
#include "Rinex3NavData.hpp"
#include "Rinex3NavStream.hpp"

// Classes for handling RINEX files with meteorological parameters
#include "RinexMetBase.hpp"
#include "RinexMetData.hpp"
#include "RinexMetHeader.hpp"
#include "RinexMetStream.hpp"

// Class for handling tropospheric models
#include "TropModel.hpp"

// Class for storing "broadcast-type" ephemerides
#include "GPSEphemerisStore.hpp"


// Class defining GPS system constants
#include "GNSSconstants.hpp"
#include"ExtractCombinationData.hpp"
#include"ModeledPR.hpp"
#include"MOPSWeight.hpp"
#include"SolverWMS.hpp"
//ephemerides store
#include"SP3EphemerisStore.hpp"
//
#include"XYZ2NEU.hpp"
#include"RequireObservables.hpp"
//
#include"SimpleFilter.hpp"
// Class to detect cycle slips using LI combination
#include "LICSDetector2.hpp"

// Class to detect cycle slips using the Melbourne-Wubbena combination
#include "MWCSDetector.hpp"

// Class to compute the effect of solid tides
#include "SolidTides.hpp"

// Class to compute the effect of ocean loading
#include "OceanLoading.hpp"

// Class to compute the effect of pole tides
#include "PoleTides.hpp"

// Class to correct observables
#include "CorrectObservables.hpp"

// Class to compute the effect of wind-up
#include "ComputeWindUp.hpp"

// Class to compute the effect of satellite antenna phase center
#include "ComputeSatPCenter.hpp"

// Class to compute the tropospheric data
#include "ComputeTropModel.hpp"

// Class to compute linear combinations
#include "ComputeLinear.hpp"

// This class pre-defines several handy linear combinations
#include "LinearCombinations.hpp"

// Class to compute Dilution Of Precision values
#include "ComputeDOP.hpp"

// Class to keep track of satellite arcs
#include "SatArcMarker.hpp"

// Class to compute gravitational delays
#include "GravitationalDelay.hpp"

// Class to align phases with code measurements
#include "PhaseCodeAlignment.hpp"

// Compute statistical data
#include "PowerSum.hpp"

// Used to delete satellites in eclipse
#include "EclipsedSatFilter.hpp"

// Used to decimate data. This is important because RINEX observation
// data is provided with a 30 s sample rate, whereas SP3 files provide
// satellite clock information with a 900 s sample rate.
#include "Decimate.hpp"

// Class to compute the Precise Point Positioning (PPP) solution
#include "SolverPPP.hpp"
#include"SolverPPPFB.hpp"

#include"BasicModel.hpp"

#include"CommandOption.hpp"
#include"BasicFramework.hpp"
#include"ConfDataReader.hpp"

#include"ProcessingList.hpp"
//
#include"actions.h"
#include"autonomus.h"
#include"auxiliary.h"
#include"PRSolverLEO.h"
#include"PRSolver.h"

// TODO: reference additional headers your program requires here

#endif // !STDAFX_HPP