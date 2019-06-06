/*  Copyright 2017 Lambert Rubash

    This file is part of TopNetCpp, a translation and enhancement of
    Fortran TopNet.

    TopNetCpp is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    TopNetCpp is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with TopNetCpp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "topnet.hh"
#include <dirent.h>
#include <iomanip>
#include <sstream>

using namespace std;

ofstream topErrorFile;
struct tm *timeinfo;
time_t startAnnual;

// ***********************************************************************

// This version, V2, has enhanced checking for missing data in the RAMS input and rainfilling
// V3 also has checking/matching of the flow sites with reaches.
int hyData(int &sDate, int &sHour, long long int &interval, int &m, int &mi, int &mps, int &mpe, int &Ngauge, int &Neq,
	vector<vector<double> >  &bRain, double **flow, int &iret, valarray<double> &dewp, valarray<double> &trange, double **dtBar,
	const int Ns, vector<vector<double> >&wrg, vector<vector<int> > &lrg, const valarray<double> &elevtg, double **bTmax, double **bTmin, double
	**bTdew, double **bdtBar, const vector<vector<double> > &Sp, const int maxGauge, const int maxInt, const int maxSites,
	const int maxResponse, const int maxTGauge, valarray<double> &wind2m, vector<vector<double> > &wrg1,
	int &idebugoutput, int &idebugbasin, int &idebugcase, const string calledFrom)   // Note that temper was passed in here but has has no use.
{
	int date, year, month, day, hour, minutes, seconds;
	int  ntri, jj, ij;
	int  js, kg;

    time_t rawtime;

	long long int itemp1, itemp2, itemp3;
	int kk;
	vector<double> tempr(maxSites), tempr_last(maxSites);
	vector<vector<double> > tempt(maxSites,vector<double>(3));

	vector<double> tempf, tempf_last;

	int i, j, nwind, wsite[1];
	//	integer*4 dsite(maxsites) dsite holds the flow site no's
	int nfill[maxGauge];
	int rsite[maxGauge], fillflag=0;
	int tsite[maxGauge];
	string rainfill_file, lineString;
	istringstream line;
	string verno;
	int it_save_old;
	bool i_reset_flag=false, exist;
	struct dirent *dirp;
	DIR *dp;
	string testStr, inLine;

	vector<vector<double> > rcoeff(maxGauge,vector<double>(maxGauge));
	vector<vector<int> > fsite(maxGauge,vector<int>(maxGauge));

	//	REAL*4 RAIN_FACTOR           defined in globaly in namespace rain
	//	COMMON /RAIN1/ RAIN_FACTOR

#if TRACE
	static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
	string save_caller = caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> hyData(" << ncalls << ")" << std::endl;
    }
	caller = "hyData";
	double tm1; // This is needed due to spaghetti code below.
#endif

    topErrorFile.open("results/toperror.txt");

	//------------ topinp.dat -------------------
	std::istringstream iss;
	ifstream topinpFile;
	string topinpFileName;
#ifdef WRIA1
	topinpFileName = "topinpWRIA1.dat";
#else
    topinpFileName = "topinp.dat";
#endif
    topinpFile.open(topinpFileName);
	if (topinpFile.good()) {
        cout << calledFrom << ": hyData(): " << topinpFileName << " opened for reading.\n";
    } else {
        cout << calledFrom << ": hyData(): " << topinpFileName << " not found.\n";
        exit(1);
    }
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> sDate;
    iss.clear();
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> sHour;
    iss.clear();
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> interval;
    iss.clear();
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> m;
    if (m > maxInt) {
        cerr << " The simulation length of " << dec << setw(5) << m;
        cerr << " timesteps exceeds the program limit of " << dec << setw(7) << maxInt;
        cerr << " The program limit will be used.\n";
    }
    if (m <= 0) {
        cerr << " The simulation length < 1, see topinp.dat\n";
        exit(0);
	}
	iss.clear();
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> mps;
    if (mps <= 0) {
        mps = 1;
    }
    if (mps > m) {
        mps = m;
    }

    iss.clear();
    getline(topinpFile, inLine, '\n');
    iss.str(inLine);
    iss >> mpe;
    idebugbasin = 0;
	idebugcase  = 0;
    getline(topinpFile, inLine, '\n');
    iss.clear();
    iss.str(inLine);
	iss >> idebugoutput;
	if(idebugoutput != 0) {
        iss >> idebugbasin;
        iss >> idebugcase;
    }
    topinpFile.close();

	year     = sDate/10000;
	day      = sDate - year*10000;
	month    = day/100;
	day     -= month*100;
    hour     = sHour/10000;
	seconds  = sHour - 10000*hour;
	minutes  = seconds/100;
	seconds -= minutes*100;
    // Assuming that sHour = 240000 means that data is for the past 24 hours
    // then tm_hour = 0 is the model start time needed here.

    // get current timeinfo and modify it
    time( &rawtime );
    //timeinfo = localtime ( &rawtime );
    timeinfo = gmtime( &rawtime );
    timeinfo->tm_year = year - 1900;// years since 1900
    timeinfo->tm_mon = month - 1;   // months since January	0-11
    timeinfo->tm_mday = day;        // day of the month	1-31
    timeinfo->tm_hour = 0;          // hours since midnight	0-23
    timeinfo->tm_min = minutes;     // minutes after the hour	0-59
    timeinfo->tm_sec = seconds;     // seconds after the minute	0-60
    timeinfo->tm_zone = "UTC";
    timeinfo->tm_isdst = -1;        // we don't want daylight saving time here

    // call mktime: timeinfo->tm_wday will be set
    startAnnual = mktime ( timeinfo );
    cout << calledFrom << ": hyData(): Model starting date/time is: " << asctime(timeinfo);
	//------------ topinp.dat -------------------

	td8micsec(sDate, sHour, itemp2);

	mi = 0;
	for (i = 1; i <= maxGauge; i++) {
		rsite[i-1] = i;
	}
	string rainFileName;
#ifdef WRIA1
    rainFileName = "rain_allWRIA1.dat";
#else
    rainFileName = "rain.dat";
#endif
	rainFile.open(rainFileName);		// fortran unit lung
	if (!rainFile.is_open()) {
		cerr << calledFrom << ": hyData(): Failed to open " << rainFileName << '\n';
		exit(EXIT_FAILURE);
	} else {
        cerr << calledFrom << ": hyData(): " << rainFileName << " opened for reading.\n";
    }
	getline(rainFile, inLine, '\n');
	getline(rainFile, inLine, '\n');
	rainFile >> verno;

	if (verno == "Ver1" || verno == "ver1" || verno == "Ver2" || verno == "ver2") {
		rainFile >> ntri;
		for (i = 1; i <= ntri; i++) {
			rainFile >> rsite[i-1];
		}
	} else {
		for (i = 1; i <= ntri; i++) {
			rainFile >> rsite[i-1];
		}
		rainFile >> Ngauge;
		ntri = Ngauge;	// need to define ntri for changes below
	}
	rainFile.close();

	for (i = 0; i < maxGauge; i++) {
		for (j = 0; j < maxGauge; j++) {
			fsite[i][j] = 0;
		}
		nfill[i] = 0;
	}

	rainfill_file = "rainfill.txt";
	exist = false;
	if ((dp = opendir(".")) == NULL) {
		cerr << "Can't open directory\n";
		exit(EXIT_FAILURE);
	}
	while ((dirp = readdir(dp)) != NULL) {
		testStr = dirp->d_name;
		if (testStr == rainfill_file) {
			exist = true;
			closedir(dp);
			break;
		}
	}
	if (exist == false) {
		cerr << calledFrom << ": hyData(): No " << rainfill_file << " file found, continuing without it.\n";
	} else {
		rainfill_read(rainfill_file, ntri, rsite, rcoeff, fsite, nfill, fillflag, maxGauge);
		if (fillflag < 0) {
            cerr << calledFrom << ": hyData():  Empty " << rainfill_file << " file found, continuing without it.\n";
		}
	}
	date = 0;
	hour = 0;
	rainFile.open(rainFileName);		// fortran unit lung
	if (!rainFile.is_open()) {
		cerr << calledFrom << ": hyData(): Failed to open " << rainFileName << '\n';
		exit(EXIT_FAILURE);
	} else {
        cerr << calledFrom << ": hyData(): " << rainFileName << " opened for reading.\n";
	}
	getline(rainFile, inLine, '\n');
	getline(rainFile, inLine, '\n');
	getline(rainFile, inLine, '\n');
	i = 0;
L203: for (jj = 0; jj < ntri; jj++) {
		rainFile >> tempr[jj];
	}
	rainFile >> date >> hour;
    td8micsec(date, hour, itemp1);
	if (i == 0) {
		// data starts after given start - flag as an error
		if ( itemp2 < itemp1-interval) {    // interval is the timestep interval in seconds
			cerr << calledFrom << ": hyData():  Start time of the rainfall, or runoff data " << dec << setw(9) << date;
			cerr << dec << setw(7) << hour;
			cerr << " is after model start at " << dec << setw(9) << sDate;
			cerr << dec << setw(7) << sHour << ". Exiting.\n";;
			exit(EXIT_FAILURE);	// iret != 0 in the fortran code stops the program
		}
		// present data is before given start - go and read the next value
		if (itemp2 > itemp1)
			goto L203;
	}
	i++;
	// added some code here to allow for breaks in the rain.dat file
	if (i == 1) {  // First time through so need to set up values for itemp3 & TEMPR_LAST
		itemp3 = itemp1 - interval;
	}

	if (itemp1-itemp3 != interval) {
		if (itemp1-itemp3 < interval) {
			cerr << calledFrom << ": hyData(): Check data in " << rainFileName << " near " << date << " " << hour << '\n';
			cerr << "Exiting.\n";
			exit(EXIT_FAILURE);
		}
		cerr << calledFrom << ": hyData(): Data missing in " << rainFileName << " near " << date << " " << hour << '\n';
		cerr << calledFrom << ": hyData(): Missing data replaced with last given value\n";
		itemp3 += interval;
		for (jj = 0; jj < ntri; jj++) {
			tempr[jj] = tempr_last[jj];
		}
	}

		// Fill missing rainfall for this timestep: TEMPr overwritten
		// don't try to fill missing data unless we found a rainfill.txt
		// need to come here even if rainfall ends prematurely to ensure filling.
	// ******************** ignore filling gaps initially RPI 17/3/2004
//L220:
    if (fillflag > 0) {
		rainfill_doit(tempr, i, ntri, rcoeff, fsite, nfill, i_reset_flag, it_save_old, maxGauge);
	}

	// Calculate basin rainfalls - code transferred from CALCTS
	for (js = 0; js < Ns; js++) {				// for each subbasin
		bRain[js][i-1] = 0.0;		 			// "brain" has basin rain time series
		for (kg = 1; kg <= maxGauge; kg++) {	// get brain by adding weighted "rain"
			if (lrg[js][kg-1] > 0) {
				bRain[js][i-1] += tempr[lrg[js][kg-1]-1]*wrg[js][kg-1]; // mm to um
			}
		}
	}

	itemp3 = itemp1;	// Finished filling, so update itemp3 AND TEMPR_LAST in case neede at next time step
	for (jj = 0; jj < ntri; jj++) {
		tempr_last[jj] = tempr[jj];
	}
	// reset number of values if missing data at end of record
	if ( i_reset_flag ) {
		m = it_save_old;
		cerr << calledFrom << ": hyData(): +++++++++++ WARNING WARNING ++++++++++++\n";
		cerr << calledFrom << ": hyData(): ***** Owing to missing rainfall at all stations to the end of the record\n";
		cerr << calledFrom << ": hyData(): it is not possible to reliably estimate flows beyond interval " << dec << setw(5) << it_save_old << ",\n";
		cerr << calledFrom << ": hyData(): therefore M reset accordingly\n";
	}
	if ( i < m  )
		goto L203;
	rainFile.close();
	// RAIN_FACTOR read in MDDATA
	// from MODELSPC.DAT and the multiplication is done here to because we have
	// to wait until Ross has done any filling required.
	for (i = 0; i < m; i++) {
		for (jj = 0; jj < Ns; jj++) {
			bRain[jj][i] *= rain::rain_factor;
		}
	}

	date = 0;
	hour = 0;

	ifstream windFile("wind.dat");		// fortran unit lung
	if (!windFile.is_open()) {
		cerr << calledFrom << ": hyData(): Failed to open wind.dat. Exiting.\n";
		exit(EXIT_FAILURE);
	}

	getline(windFile, inLine, '\n');
	getline(windFile, inLine, '\n');
	windFile >> verno >> nwind;
	for (i = 1; i <= nwind; i++) {
		windFile >> wsite[i-1];
	}
	getline(windFile, inLine, '\n');	// read the rest of the line
	i = 0;
L3103: if (!(windFile >> tempr[0] >> date >> hour))
		goto L3101;

	td8micsec(date, hour, itemp1);
	if ( i == 0 ) {
		// data starts after given start - flag as an error
		if ( itemp2 < itemp1-interval )
			goto L3100;
		// present data is before given start - go and read the next value
		if ( itemp2 > itemp1 )
			goto L3103;
	}
	i++;
	wind2m[i-1] = tempr[0];
	if (i == 1) {  // First time through so need to set up values for itemp3 & TEMPR_LAST
		itemp3 = itemp1 - interval;
	}
	if (itemp1-itemp3 != interval) {
L1682:	if (itemp1-itemp3 < interval) {
				cerr << calledFrom << ": hyData(): Check data in wind.dat near " << date << " " << hour << ". Exiting.\n";;
				exit(EXIT_FAILURE);
		}
		cerr << calledFrom << ": hyData(): Data missing in wind.dat near " << date << " " << hour << '\n';
		cerr << calledFrom << ": hyData(): Missing data replaced with last given value\n";
		itemp3 = itemp3 + interval;
		i++;
		if (itemp1-itemp3 != interval)
			goto L1682;
	}
	itemp3 = itemp1; // Finished filling, so update itemp3 AND TEMPR_LAST in case neede at next time step

	if (i < m)
		goto L3103;
	windFile.close();
	goto L3104;
L3100: cerr << calledFrom << ": hyData():  ***** Warning: wind.dat does not exist or contains an error at ";
	cerr << dec << setw(9) << date << setw(7) << hour << '\n';
	cerr << calledFrom << ": hyData():  Proceeding with constant temperature\n";
L3101: if (m != i) {
		m = i;
        cerr << calledFrom << ": hyData(): Warning: Temperature file Ends prematurely. Number of time steps reduced to " << m << '\n';
	}
	if ( m <= 0 ) {
		cerr << calledFrom << ": hyData():  **** Change to length of temperature data has caused";
		cerr << " a value of data length < 1 - check simulation dates in";
		cerr << " topinp.dat and temper.dat\n";
		exit(EXIT_FAILURE);
	}
L3104: ;
	// put in default temperatures if there's no temperature data
	for (i = 0; i < m; i++) {
		//temper[i] = 10.0; // not used
		dewp[i]   = 7.0;
		trange[i] = 10.0;
	}
	i = m;
	date = 0;
	hour = 0;
	string tmaxtmintdewFileName;
#ifdef WRIA1
    tmaxtmintdewFileName = "tmaxtmintdew_allWRIA1.dat";
#else
    tmaxtmintdewFileName = "tmaxtmintdew.dat";
#endif
	ifstream tmaxtmintdewFile(tmaxtmintdewFileName);		// fortran unit lung
	if (!tmaxtmintdewFile.is_open()) {
		cerr << calledFrom << ": hyData(): Failed to open " << tmaxtmintdewFileName << ". Exiting.\n";
		exit(EXIT_FAILURE);
	}

	getline(tmaxtmintdewFile, inLine, '\n');
	getline(tmaxtmintdewFile, inLine, '\n');
	tmaxtmintdewFile >> verno >> ntri;
	for (i = 1; i <= ntri; i++) {
		tmaxtmintdewFile >> tsite[i-1];
	}
	getline(tmaxtmintdewFile, inLine, '\n');	// read the rest of the line
	i = 0;
L303: for (jj = 0; jj < ntri; jj++) {
		for (js = 0; js < 3; js++) {
			if (!(tmaxtmintdewFile >> tempt[jj][js]))
				goto L301;
		}
	}
	tmaxtmintdewFile >> date >> hour;
	td8micsec(date, hour, itemp1);
	if ( i == 0 ) {
		// data starts after given start - flag as an error
		if ( itemp2 < itemp1-interval )
			goto L300;
		// present data is before given start - go and read the next value
		if ( itemp2 > itemp1 )
			goto L303;
	}
	i++;
	// added some code here to allow for breaks in the input file
	if (i == 1) {  // First time through so need to set up values for itemp3 & TEMPR_LAST
		itemp3 = itemp1 - interval;
	}
	if (itemp1-itemp3 != interval) {
L682: 	if (itemp1-itemp3 < interval) {
			cerr << calledFrom << ": hyData():  Check data in tmaxtmintdew.dat near " << date << hour << ". Exiting.\n";
			exit(EXIT_FAILURE);
		}
		cerr << calledFrom << ": hyData():  Data missing in tmaxtmintdew.dat near " << date << hour << '\n';
		cerr << " Missing data replaced with last given value\n";
		itemp3 = itemp3 + interval;
		//  4/23/07  DGT added code below to implement missing within IF
		for (js = 0; js < Ns; js++) {			 // for each subbasin
			bTmax[js][i-1] = 0.0;		 // "bTmax" has basin tmax time series
			bTmin[js][i-1] = 0.0;		 // "bTmin" has basin tmax time series
			bTdew[js][i-1] = 0.0;		 // "bTdew" has basin tmax time series
			for (kg = 1; kg <= maxTGauge; kg++) { // get bTmax by adding using lapse to get from gauge_elev to basin_elev, plus spatial weighting on tmax
				if (lrg[js][kg-1] > 0) {  // Interpolation weights
            	    bTmax[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]][0] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
            	    bTmin[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]][1] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
            	    bTdew[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]][2] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
				}
			}
		}
		i++;
		if (itemp1-itemp3 != interval)
			goto L682;
	}
	// Calculate basin temps - code adapted from basin rain raw 9-dec-2004
 	for (js = 0; js < Ns; js++) {			 // for each subbasin
		bTmax[js][i-1] = 0.0;		 // "bTmax" has basin tmax time series
		bTmin[js][i-1] = 0.0;		 // "bTmin" has basin tmax time series
		bTdew[js][i-1] = 0.0;		 // "bTdew" has basin tmax time series
		for (kg = 1; kg <= maxTGauge; kg++) { // get bTmax by adding using lapse to get from gauge_elev to basin_elev, plus spatial weighting on tmax
			if (lrg[js][kg-1] > 0) {  //  Interpolation weights
				bTmax[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]-1][0] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
				bTmin[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]-1][1] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
				bTdew[js][i-1] += wrg1[js][kg-1]*(tempt[lrg[js][kg-1]-1][2] + Sp[12][js]*(elevtg[lrg[js][kg-1]-1] - Sp[13][js]));
			}
		}
 	}
	itemp3 = itemp1; // Finished filling, so update itemp3 and tempr_last in case neede at next time step
	if ( i < m )
		goto L303;

	for (js = 0; js < Ns; js++) {
		for (i = 0; i < 12; i++) {
			bdtBar[i][js] = 0;
		}
		for (kg = 1; kg <= maxTGauge; kg++) { // get basin average dtbar by averaging dtbar values for gauges
			if (lrg[js][kg-1] > 0) {
				for (kk = 1; kk <= 12; kk++) {
					bdtBar[kk-1][js] += wrg1[js][kg-1]*dtBar[kk-1][lrg[js][kg-1]-1];  // WRG1 Interpolation weights
				}
			}
		}
	}

	tmaxtmintdewFile.close();
	goto L304;
L300: cerr << calledFrom << ": hyData(): ***** Warning: temper.dat does not exist or contains an error at ";
	cerr << dec << setw(9) << date << setw(7) << hour << '\n';
	cerr << calledFrom << ": hyData(): Proceeding with constant temperature\n";
L301: if (m != i) {
		m = i;
        cerr << calledFrom << ": hyData(): Warning: Temperature file Ends prematurely\n";
		cerr << calledFrom << ": hyData(): number of time steps set to " << m << '\n';
	}
	if ( m <= 0 ) {
		cerr << calledFrom << ": hyData(): **** Change to length of temperature data has caused";
		cerr << " a value of data length < 1 - check simulation dates in";
		cerr << " topinp.dat and temper.dat\n";
		exit(EXIT_FAILURE);
	}
L304: ;

	// put in zero runoff in case there's no runoff data (e.g. flood forecasting)
	for (jj = 1; jj <= maxResponse; jj++) {
		for (i = 1; i <= m; i++) {
	    	flow[jj-1][i-1] = 0.0;
		}
	}
	i = m;
	date = 0;
	hour = 0;

	ifstream streamflow_calibrationFile("streamflow_calibration.dat");		// fortran unit lung
	if (!streamflow_calibrationFile.is_open()) {
		cerr << calledFrom << ": hyData(): Failed to open streamflow_calibration.dat.\n";
		goto L2031;
	}

	getline(streamflow_calibrationFile, inLine, '\n');
	getline(streamflow_calibrationFile, inLine, '\n');
	getline(streamflow_calibrationFile, inLine, '\n');
	streamflow_calibrationFile >> verno;
	// rewind
	streamflow_calibrationFile.clear();
	streamflow_calibrationFile.seekg(0);

	if (verno == "Ver1" || verno == "ver1" || verno == "Ver2" || verno == "ver2") {
		getline(streamflow_calibrationFile, inLine, '\n');
		getline(streamflow_calibrationFile, inLine, '\n');
		getline(streamflow_calibrationFile, inLine, '\n');
		streamflow_calibrationFile >> verno >> ntri;
		getline(streamflow_calibrationFile, inLine, '\n');	// read the rest of the line
		if (ntri != Neq) {
			cerr << calledFrom << ": hyData(): *****ERROR - Number of sites in streamflow_calibration.dat " << ntri;
			cerr << " does not match what modelspc.dat is expecting, " << Neq << "\n";
			cerr << calledFrom << ": hyData(): Setting Neq = ntri\n";
			Neq = ntri;
		}
		tempf.resize(ntri,0.0);
		tempf_last.resize(ntri,0.0);
	} else {
		getline(streamflow_calibrationFile, inLine, '\n');
	}
	i = 0;
L207: for (jj = 1; jj <= Neq; jj++) {
		if (!(streamflow_calibrationFile >> tempf[jj-1])) {
            if (streamflow_calibrationFile.eof()) {
                goto L206;
            } else {
                goto L204;
            }
        }
	}
	if (!(streamflow_calibrationFile >> date >> hour)) {
        if (streamflow_calibrationFile.eof()) {
            goto L206;
        } else {
            goto L204;
        }
    }

	td8micsec(date, hour, itemp1);
	topErrorFile << dec << setw(4) << i;
	topErrorFile << dec << setw(4) << Neq;
	topErrorFile << dec << setw(6) << m;
	topErrorFile << fixed << setw(9) << setprecision(3) << tempf[0];
	topErrorFile << fixed << setw(9) << setprecision(3) << tempf[1];
	topErrorFile << dec << setw(10) << date;
	topErrorFile << dec << setw(8) << hour;
	topErrorFile << dec << setw(12) << itemp1;
	topErrorFile << dec << setw(12) << itemp2 << endl;

	if ( i == 0 ) {
		// data starts after given start - flag as an error
		if ( itemp2 < itemp1-interval )
			goto L230;
		// present data is before given start - go and read the next value
		if ( itemp2 > itemp1 )
			goto L207;
	}
	i++;
	// added some code here to allow for breaks in the RUNOFF.DAT file
	if (i == 1) {  // First time through so need to set up values for itemp3 & tempf_LAST
			itemp3 = itemp1 - interval;
	}
	if (itemp1-itemp3 != interval) {
L782:	if (itemp1-itemp3 < interval) {
			cerr << calledFrom << ": hyData():  Check data in streamflow_calibration.dat near ";
			cerr << dec << setw(9) << date << setw(7) << hour << ". Exiting.\n";
			exit(EXIT_FAILURE);
		}
		cerr << calledFrom << ": hyData():  Data missing in streamflow_calibration.dat near ";
		cerr << dec << setw(9) << date << setw(7) << hour << '\n';
		cerr << calledFrom << ": hyData():  Missing data replaced with last given value\n";
		itemp3 = itemp3 + interval;
		for (jj = 0; jj < Neq; jj++) {
			flow[jj][i-1] = tempf_last[jj]/1000.0;
		}
		i++;
		if (itemp1-itemp3 != interval)
			goto L782;
	}
	itemp3 = itemp1; // Finished filling, so update itemp3 AND tempf_LAST in case neede at next time step
	for (jj = 0; jj < Neq; jj++) {
		tempf_last[jj] = tempf[jj];
	}

	for  (jj = 0; jj < Neq; jj++) {
		flow[jj][i-1] = tempf[jj]/1000.0;
	}
	if ( i < m  ) {
		goto L207;
	}
	streamflow_calibrationFile.close();
	//	close(20) not closed so that top1 can read from this
L2031: if (m != i) {
		cerr << calledFrom << ": hyData(): ***** Warning: streamflow_calibration.dat does not exist or contains an error at ";
		cerr << dec << setw(9) << date << setw(7) << hour << '\n';
		cerr << calledFrom << ": hyData():  Proceeding with measured runoff = 0\n";
	}
#if TRACE
	tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving hyData @ 1(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif
	return 0;

//L250:  cerr << "Error or End of file reading topinp.dat\n";
//	iret = 4;
//	return 0;
//L200:  cerr << "***** Error in rain.dat or streamflow_calibration.dat at ";
//	cerr << dec << setw(9) << date << setw(7) << hour << '\n';
//	iret = 4;
//	return 0;
//L201: cerr << " ***** Rainfall file is empty\n";
//	iret = 4;
//	return 0;
//L202: m = i;
//	if ( m <= 0 ) {
//		cerr << " **** Premature end to data has caused a value of data";
//		cerr << " length < 1 - check simulation dates in topinp.dat and/or";
//		cerr << " the dates in rain.dat and streamflow_calibration.dat";
//		exit(EXIT_FAILURE);
//	}
//	cerr << " ***** Rainfall ends prematurely - number of values reduced to " << dec << setw(6) << m << '\n';
//	goto L220;
L204: cerr << " ***** Error in file streamflow_calibration.dat at ";
	cerr << dec << setw(9) << date << setw(7) << hour << '\n';
	iret = 4;
	return 0;
L206: for (ij = i+1; ij <= m; ij++) {
 		for (jj = 1; jj <= Neq; jj++) {
 			flow[jj-1][ij-1] = -1.0;
 		}
	}
 	cerr << calledFrom << ": hyData():  ***** RUNOFF DATA ENDS PREMATURELY - Flows set to -1 - Ignore this message if forecasting\n";
	if ( m <= 0 ) {
		cerr << calledFrom << ": hyData():  **** Premature end to data has caused a value of data";
		cerr << " length < 1 - check simulation dates in topinp.dat and/or";
		cerr << " the dates in rain.dat and streamflow_calibration.dat. Exiting.\n";
		exit(EXIT_FAILURE);
	}
	// need to close this because we will be opening it again later
	streamflow_calibrationFile.close();
L230:
#if TRACE
	tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving hyData @ 2(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif

	return 0;
}
