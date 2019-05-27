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
#include <sstream>
#include <iomanip>
#include "snow.hh"

using namespace std;

ofstream oFile[51], extraFiles[13];
ofstream snowcontrol3_File;	// unit 10

// This version,V3, has lakes and snow modelling added to it

// ******************************************************
// *  subroutine  calcts
// ******************************************************
int calcts( double **Si,            const vector<vector<double> > &Sp,  double **Rp,                    const valarray<int> &ll,
            const int Nsub,         const valarray<int> &Nka,           const valarray<double> &tl,     double **atb,
            double **pka,           const valarray<int> &nd,            double **cl,                    double **pd,
            const double units,     const int ipsub,                    const int ipatb,                const bool reinit,
            const bool modwrt,      const int stim,                     vector<vector<double> > &bRain, const long int interval,
            const int m,            const int mi,                       const int mps,                  const int mpe,
            bool &ok,               const valarray<double> &xlat,       const valarray<double> &xlong,  const double stdlon,
            const valarray<double> &elevtg,   double **bdtBar,                    const int sDate,                int &sHour,
            const valarray<double> &temper, const valarray<double> &dewp, const valarray<double> &tRange, const int Neq,
            const int Nout,         const int nBout,                    const valarray<int> &iBout,     const valarray<double> &wind2m,
            double **bTmin,         double **bTmax,                     double **bTdew,                 const valarray<double> &bXlat,
            const valarray<double> &bXlon,    int *ntdh,                          const int ndump,                const int maxInt,
            const int maxSlp,       const int maxA,                     const int maxC,                 const int maxChn,
            const int idebugoutput, const int idebugbasin,              const int idebugcase)
{
    istringstream line;
    char dateStr[11];
    double daysInYear = -1.0;
    const int offset = 18;            // Offset needed for file name array.
    const int withdrawalOffset = 39;  // Offset for withdrawal by user type file output code.
    const int NuserTypes = 6;
    ostringstream TcodeStr, AcodeStr;
    int ncode;

    int userTypeCode[] = {constant_definitions::SoilMoistureIrrigationUseCode,      // 1
                          constant_definitions::FixedDemandIrrigationUseCode,       // 2
                          constant_definitions::DownstreamReservoirReleaseUseCode,  // 3
                          constant_definitions::PWSUseCode,                         // 4
                          constant_definitions::NonPWSMandIUseCode,                 // 5
                          constant_definitions::DairyUseCode,                       // 6
                          constant_definitions::RanchUseCode,                       // 7
                          constant_definitions::PoultryUseCode,                     // 8
                          constant_definitions::ParkGolfCemeteryUseCode,            // 9
                          constant_definitions::InstreamFlowUseCode,                // 10
                          constant_definitions::DiversionUseCode};                  // 11

    // The Model
    int ievap_method, n_irrig, n_drainage, kk, i_irrig, i_drainage, kp;
    double wt0, wt1, wt2, wt12, depth_irrig_dem, rate_irrig, art_drainage;
    double dep, qinst_out_0, dr_out_d, dr_out_0;
    double zbm_d, acsem_d, s1_d, sr_d, cv_d, bal_d, qinst_out_d;
    double zbar_d;
    double sumr_d, sumq_d, sumae_d, s0_d, sumpe_d, dth1;
    double art_drainage_out;   // artificial drainage
    double scalefactor;

    // DGT 5/27/12 allocating arrays that seemed to be allocated dynamically before
    double *groundwater_to_take; 					// groundwater_to_take(maxSlp)
    double evap_mm, qlat_mm;
    const int nreg = 6;								// how many regions can we model within a sub-basin?
    // use these regions for irrigation and drainage

    //flood comment out next line
    static vector<int> irr_l(Nip1);
    static array<double,Nip1> rirr;
    static int istep, j, n;

    //   locals
    int js;
    //   ET variables
    static double albedo, elevsb, rlapse;
    static double PETsngl, elev;
    static double temper_temp, dewp_temp, trange_temp;
    // snow
    static double ddf;
    static int irad;

    //         REACH ROUTING ( ROUTE )
    static int jr, jr1;

    // locals:
    static double prec, pet=0.0;
    static double *snowst;

    int jsub, i;
    static int jj;

    static double smin;

    // This is special for CALCTS
    static int  iyear, month = 0, iday, ihr, imm, isec, ihh, iss;
    static double hour1;
    static double **sr, **cv;
    static double *q0, **s0;
    static double **tdh;
    static double **aciem;
    static double **acsem;
    static double **zbm;
    static double **sumr;
    static double **sumae;
    static double **sumpe;
    static double **sumq;
    static double **sumie;
    static double **sumse;
    static double **sumqb;
    static double **sumce;

    static double sumad;
    static double **sumsle;
    static double **sumr1;
    static double **sumqv;

    static double *qb;
    static double *zr;
    static double *ak0fzrdt;
    static double *logoqm;
    static double *qvmin;
    static double *dth;

    static int ngut, natball;

    // SNOWUEB
    static int ndepletionpoints, ipflag, mQgOption;
    static int nintstep, isurftmpoption, nstepday, isnow_method;
    static double bca, bcc, a, b, timestep;
    double surfacewaterinput;
    static double snowevaporation, areafractionsnow;
    static double ta, p, ws, qsiobs, qnetob;
    static string inFile, outFileName, pFile, svFile, bcFile;
    static string aFile, dfcFile, outFileDet;
    static string inLine;
    static array<double,Nsv> snowsitev;
    //double **snowstatev;

    static array<double,Niv> snowforcing;
    static vector<double> snowparam(Npar);
    static array<int,7> snowcontrol;
    static array<double,12> dtbar;
    static double *cump;
    static double *cume;
    static double *cummr;
    static double *errmbal;
    static double *w1;

    // arrays to keep records of surface temperature and snowpack average
    // temperature. This is for the fourth model (Modified force restore approach)
    static double **dfc;

#if TRACE
    static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    string save_caller = caller;
    if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> calcts(" << ncalls << ")" << std::endl;
    }
    caller = "calcts";
#endif
    string resultsFileNames[] = {"results/annual_upwelling_mm.txt",                      // 18
                                 "results/annual_recharge_mm.txt",                       // 19
                                 "results/annual_precipitation_mm.txt",                  // 20
                                 "results/annual_potential_evapotranspiration_mm.txt",   // 21
                                 "results/annual_return_flow_cmy.txt",                   // 22, cubic meters per year

                                 "results/monthly_upwelling_mm.txt",                     // 23
                                 "results/monthly_recharge_mm.txt",                      // 24
                                 "results/monthly_precipitation_mm.txt",                 // 25
                                 "results/monthly_potentialevap_mm.txt",                 // 26
                                 "results/monthly_return_flow_cmm.txt",                  // 27, cubic meters per month

                                 "results/annual_mean_upwelling_mm.txt",                 // 28
                                 "results/annual_mean_recharge_mm.txt",                  // 29
                                 "results/annual_mean_precipitation_mm.txt",             // 30
                                 "results/annual_mean_potentialevap_mm.txt",             // 31
                                 "results/annual_mean_return_flow_cmy.txt",              // 32, cubic meters per year

                                 "results/annual_artificial_drainage_cmy.txt",           // 33
                                 "results/monthly_artificial_drainage_cmm.txt",          // 34
                                 "results/annual_mean_artificial_drainage_cmy.txt",      // 35

                                 "results/annual_evaporation_mm.txt",                    // 36
                                 "results/monthly_evaporation_mm.txt",                   // 37
                                 "results/annual_mean_evaporation_mm.txt"};              // 38

    string extraFileNames[] = { "results/upwelling_irrigated_natural_drainage_mm.txt",   // 0
                                "results/upwelling_irrigated_tile_drainage_mm.txt",      // 1
                                "results/upwelling_irrigated_ditch_drainage_mm.txt",     // 2
                                "results/upwelling_unirrigated_natural_drainage_mm.txt", // 3
                                "results/upwelling_unirrigated_tile_drainage_mm.txt",    // 4
                                "results/upwelling_unirrigated_ditch_drainage_mm.txt",   // 5

                                "results/recharge_irrigated_natural_drainage_mm.txt",    // 6
                                "results/recharge_irrigated_tile_drainage_mm.txt",       // 7
                                "results/recharge_irrigated_ditch_drainage_mm.txt",      // 8
                                "results/recharge_unirrigated_natural_drainage_mm.txt",  // 9
                                "results/recharge_unirrigated_tile_drainage_mm.txt",     // 10
                                "results/recharge_unirrigated_ditch_drainage_mm.txt",    // 11

                                "results/precip_minus_et_mm.txt"};                       // 12

    // cms calculation for e.g. user withdrawal is dividided by 864000 which implies that user withdrawal
    // stored in the User structure is in units of cubic meters per day. See the
    // 'write_output_tables.cpp' file for examples.

    string drainageWithdrawalTotalsFileNames[NuserTypes], drainageWithdrawalAnnAveFileNames[NuserTypes];

    build_topnet_to_client_index();     // Early initialization of TopNet to Client drainage number conversion array.
    // annual, monthly and average files must be opened and initialized here because their
    // write routines cannot look for timestep = 0
    strftime(dateStr, 11,"%Y %m %d", timeinfo);
    // --------------------------------------------------------------------------------
    for (i = 0; i < 13; ++i) {
        extraFiles[i].open(extraFileNames[i]);
        if (!extraFiles[i].is_open()) {
            cerr << "Failed to open " << extraFileNames[i] << '\n';
            exit(EXIT_FAILURE);
        } else {
            cerr << "Extra File # " << dec << setw(2) << i << " " << extraFileNames[i] << " opened" << endl;
        }
        extraFiles[i] << left << dec << setw(11) << "Date";
        for (jsub = 0; jsub < Nsub; jsub++) {
            extraFiles[i] << setw(9) << "Drainage_" << left << dec << setw(8)
                          << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;
        }
        extraFiles[i] << endl;
    }
    // --------------------------------------------------------------------------------
    for (i = offset; i < 28; ++i) {
        oFile[i].open(resultsFileNames[i-offset]);
        if (!oFile[i].is_open()) {
            cerr << "Failed to open " << resultsFileNames[i-offset] << '\n';
            exit(EXIT_FAILURE);
        } else {
            cerr << "File # " << dec << setw(2) << i << " " << resultsFileNames[i-offset] << " opened" << endl;
        }
        oFile[i] << left  << dec << setw(11) << "Date";
        for (jsub = 0; jsub < Nsub; jsub++) {
            oFile[i] << setw(9) << "Drainage_" << left << dec << setw(6)
                     << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;;
        }
        oFile[i] << '\n';
    }
    for (i = 28; i < withdrawalOffset; ++i) {
        oFile[i].open(resultsFileNames[i-offset]);
        if (!oFile[i].is_open()) {
            cerr << "Failed to open " << resultsFileNames[i-offset] << '\n';
            exit(EXIT_FAILURE);
        } else {
            cerr << "File # " << dec << setw(2) << i << " " << resultsFileNames[i-offset] << " opened" << endl;
        }
        if (i == 34 || i == 37) {
            oFile[i] << left  << setw(11) << "Months";
        } else {
            oFile[i] << left  << setw(11) << "Years";
        }
        for (jsub = 0; jsub < Nsub; jsub++) {
            oFile[i] << setw(9) << "Drainage_" << left << dec << setw(6)
                     << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;;
        }
        oFile[i] << '\n';
    }
    i = withdrawalOffset;
    for (ncode = 0; ncode < NuserTypes; ++ncode) {
        TcodeStr << "results/DrainageWithdrawalByUserType_" << userTypeCode[ncode] << "_cmy.txt";    // cubic meters per year
        AcodeStr << "results/AnnAveDrainageWithdrawalByUserType_" << userTypeCode[ncode] << "_cmy.txt";    // cubic meters per year
        oFile[i+ncode].open(TcodeStr.str());
        oFile[i+ncode+NuserTypes].open(AcodeStr.str());
        if (!oFile[i+ncode].is_open()) {
            cerr << "Failed to open " << TcodeStr.str() << '\n';
            exit(EXIT_FAILURE);
        } else {
            cerr << "File # " << dec << setw(2) << i+ncode << " " << TcodeStr.str() << " opened" << endl;
        }
        if (!oFile[i+ncode+NuserTypes].is_open()) {
            cerr << "Failed to open " << AcodeStr.str() << '\n';
            exit(EXIT_FAILURE);
        } else {
            cerr << "File # " << dec << setw(2) << i+ncode+NuserTypes << " " << AcodeStr.str() << " opened" << endl;
        }
        drainageWithdrawalTotalsFileNames[ncode] = TcodeStr.str();
        drainageWithdrawalAnnAveFileNames[ncode] = AcodeStr.str();
        TcodeStr.clear();
        TcodeStr.str("");
        AcodeStr.clear();
        AcodeStr.str("");
        oFile[i+ncode] << left  << setw(11) << "Year_end";
        if (i+ncode+NuserTypes >= 23 &&i+ncode+NuserTypes < 28) {
            oFile[i+ncode+NuserTypes] << left  << setw(11) << "Months";
        } else {
            oFile[i+ncode+NuserTypes] << left  << setw(11) << "Years";
        }
        for (jsub = 0; jsub < Nsub; jsub++) {
            oFile[i+ncode] << setw(9) << "Drainage_" << left << dec << setw(6)
                           << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;
            oFile[i+ncode+NuserTypes] << setw(9) << "Drainage_" << left << dec << setw(6)
                                      << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;
        }
        oFile[i+ncode] << '\n';
        oFile[i+ncode+NuserTypes] << '\n';
    }
    //----------------------------------------------------------------------------------------------------
    oFile[16].open("results/recharge_mm.txt");
    if (!oFile[16].is_open()) {
        cerr << "Failed to open results/recharge_mm.txt\n";
        exit(EXIT_FAILURE);
    } else {
        cerr << "results/recharge_mm.txt opened" << endl;
    }
    oFile[16] << left << dec << setw(11) << "Date";
    for (jsub = 0; jsub < Nsub; jsub++) {
        oFile[16] << setw(9) << "Drainage_" << left << dec << setw(8)
                  << input_structures::Drainage[index_to_real_DID[jsub]].RealDrainageID;
    }
    oFile[16] << endl;
    //----------------------------------------------------------------------------------------------------

    // Allocate and initialize all the now dynamic arrays
    vector<double>         baseflow(maxSlp,0.0);
    vector<double>      totalrunoff(maxSlp,0.0);

    vector<double>          tempave(maxSlp,0.0);
    vector<double>           surfro(maxSlp,0.0);
    vector<double>         canstore(maxSlp,0.0);
    vector<double>        soilstore(maxSlp,0.0);
    vector<double>            tiled(maxSlp,0.0);
    vector<double>           ditchd(maxSlp,0.0);
    valarray<double>       volume_irrig_sup(0.0,maxSlp);

    valarray<double>            ArtDrainage(0.0,maxSlp);
    valarray<double>     annual_ArtDrainage(0.0,maxSlp);
    valarray<double>    monthly_ArtDrainage(0.0,maxSlp);
    valarray<double>     annAve_ArtDrainage(0.0,maxSlp);

    valarray<double>   irrigated_natural_upwelling(0.0,maxSlp);
    valarray<double>      irrigated_tile_upwelling(0.0,maxSlp);
    valarray<double>     irrigated_ditch_upwelling(0.0,maxSlp);
    valarray<double> unirrigated_natural_upwelling(0.0,maxSlp);
    valarray<double>    unirrigated_tile_upwelling(0.0,maxSlp);
    valarray<double>   unirrigated_ditch_upwelling(0.0,maxSlp);
    double upwelling_temporary;
    valarray<double>              upwelling(0.0,maxSlp);
    valarray<double>       annual_upwelling(0.0,maxSlp);
    valarray<double>      monthly_upwelling(0.0,maxSlp);
    valarray<double>       annAve_upwelling(0.0,maxSlp);

    valarray<double>   irrigated_natural_recharge(0.0,maxSlp);
    valarray<double>      irrigated_tile_recharge(0.0,maxSlp);
    valarray<double>     irrigated_ditch_recharge(0.0,maxSlp);
    valarray<double> unirrigated_natural_recharge(0.0,maxSlp);
    valarray<double>    unirrigated_tile_recharge(0.0,maxSlp);
    valarray<double>   unirrigated_ditch_recharge(0.0,maxSlp);
    double recharge_temporary;
    double precip_minus_et_temporary;
    valarray<double>        precip_minus_et(0.0,maxSlp);
    valarray<double>               recharge(0.0,maxSlp);
    valarray<double>        annual_recharge(0.0,maxSlp);
    valarray<double>       monthly_recharge(0.0,maxSlp);
    valarray<double>        annAve_recharge(0.0,maxSlp);

    valarray<double>             returnflow(0.0,maxSlp);
    valarray<double>  GroundWaterReturnFlow(0.0,maxSlp);
    valarray<double> SurfaceWaterReturnFlow(0.0,maxSlp);
    valarray<double>      annual_returnflow(0.0,maxSlp);
    valarray<double>     monthly_returnflow(0.0,maxSlp);
    valarray<double>      annAve_returnflow(0.0,maxSlp);

    valarray<double>   precip_for_watermgmt(0.0,maxSlp);
    valarray<double>   annual_precipitation(0.0,maxSlp);
    valarray<double>  monthly_precipitation(0.0,maxSlp);
    valarray<double>   annAve_precipitation(0.0,maxSlp);

    valarray<double>          potentialevap(0.0,maxSlp);
    valarray<double>   annual_potentialevap(0.0,maxSlp);
    valarray<double>  monthly_potentialevap(0.0,maxSlp);
    valarray<double>   annAve_potentialevap(0.0,maxSlp);

    valarray<double>     evap_for_watermgmt(0.0,maxSlp);
    valarray<double>     annual_evaporation(0.0,maxSlp);
    valarray<double>    monthly_evaporation(0.0,maxSlp);
    valarray<double>     annAve_evaporation(0.0,maxSlp);

    vector<vector<double> > BasinWithdrawalByUser(NuserTypes,vector<double>(Nsub));  // first 6 user types
    vector<vector<double> > annAve_basin_withdrawal(NuserTypes,vector<double>(Nsub));  // first 6 user types
    int countFullYears = 0;

    vector<double> vol_irrig_demand(maxSlp,0.0);

    groundwater_to_take  = new double[maxSlp];
    for (i = 0; i < maxSlp; i++) {
        groundwater_to_take[i]  = 0.0;
    }

    vector<array<double,12> > bdtBarR(maxSlp);  // maxSlp rows, 12 columns
    vector<vector<double> > zbar(maxSlp,vector<double>(nreg));   // maxSlp rows, nreg columns
    snowst   = new double[maxSlp];
    q0       = new double[maxSlp];
    qb       = new double[maxSlp];
    qvmin    = new double[maxSlp];
    zr       = new double[maxSlp];
    ak0fzrdt = new double[maxSlp];
    logoqm   = new double[maxSlp];
    dth      = new double[maxSlp];
    cump     = new double[maxSlp];
    cume     = new double[maxSlp];
    cummr    = new double[maxSlp];
    errmbal  = new double[maxSlp];
    w1       = new double[maxSlp];
    sr       = new double*[maxSlp];
    cv       = new double*[maxSlp];
    s0       = new double*[maxSlp];
    aciem    = new double*[maxSlp];
    acsem    = new double*[maxSlp];
    zbm      = new double*[maxSlp];
    sumr     = new double*[maxSlp];
    sumae    = new double*[maxSlp];
    sumpe    = new double*[maxSlp];
    sumq     = new double*[maxSlp];
    sumie    = new double*[maxSlp];
    sumse    = new double*[maxSlp];
    sumqb    = new double*[maxSlp];
    sumce    = new double*[maxSlp];
    sumsle   = new double*[maxSlp];
    sumr1    = new double*[maxSlp];
    sumqv    = new double*[maxSlp];
    for (j = 0; j < maxSlp; j++) {
        sr[j]     = new double[nreg];
        cv[j]     = new double[nreg];
        s0[j]     = new double[nreg];
        aciem[j]  = new double[nreg];
        acsem[j]  = new double[nreg];
        zbm[j]    = new double[nreg];
        sumr[j]   = new double[nreg];
        sumae[j]  = new double[nreg];
        sumpe[j]  = new double[nreg];
        sumq[j]   = new double[nreg];
        sumie[j]  = new double[nreg];
        sumse[j]  = new double[nreg];
        sumqb[j]  = new double[nreg];
        sumce[j]  = new double[nreg];
        sumsle[j] = new double[nreg];
        sumr1[j]  = new double[nreg];
        sumqv[j]  = new double[nreg];
    }

    double ***dr = new double**[maxSlp];
    double ***qinst = new double**[maxSlp];
    for (js = 0; js < maxSlp; ++js) {
        dr[js] = new double*[nreg];
        qinst[js] = new double*[nreg];
        for (kk = 0; kk < nreg; ++kk) {
            dr[js][kk] = new double[MAX_NTDH];
            qinst[js][kk] = new double[MAX_NTDH];
        }
    }
    vector<double> dr1(MAX_NTDH);
    vector<double> qinst1(MAX_NTDH);

    double***zbar_in = new double**[m+1];
    for (int i = 0; i <= m; ++i) {
        zbar_in[i] = new double*[Nsub];
        for (int j = 0; j < Nsub; ++j) {
            zbar_in[i][j] = new double[nreg];
        }
    }

    tdh = new double*[MAX_NTDH];
    for (j = 0; j <= MAX_NTDH; j++) {
        tdh[j] = new double[maxSlp];
    }
    vector<vector<double> > snowstatev(maxSlp,vector<double>(Nxv));   // maxSlp rows, Nxv columns

    // *******************************************************************************
    // THIS SECTION IS RELEVANT TO THE WHOLE MODEL

    // ASSUME EVERYTHING WILL TURN OUT OK
    ok = true;

    //  Calculate sum of catchment areas above each reach
    ngut = Nsub;
    ievap_method = 2; //0=P-T, 1 and 2 are P-M
    // initialise snowueb model
    aFile = "snow.in";
    ifstream snowFile(aFile.c_str());	// unit 1
    if (!snowFile.is_open()) {
        cerr << "Failed to open " << aFile << '\n';
        exit(EXIT_FAILURE);
    }
    snowFile >> inFile >> outFileName >> outFileDet >> pFile >> svFile >> bcFile >> dfcFile;
    outFileName = "results/" + outFileName;
    snowFile >> irad;
    getline(snowFile, inLine, '\n');           // Read the remainder of the line
    //   Flag to control radiation
    //    0 is no measurements - radiation estimated from diurnal temperature range
    //    1 is incoming shortwave radiation read from file (measured), incoming longwave estimated
    //    2 is incoming shortwave and longwave radiation read from file (measured)
    //    3 is net radiation read from file (measured)
    snowFile >> ipflag;  						// Flag to control printing (1=print)
    getline(snowFile, inLine, '\n');			// Read the remainder of the line
    snowFile >> nintstep;  						// Number of internal time steps to use
    getline(snowFile, inLine, '\n');			// Read the remainder of the line
    snowFile >> isurftmpoption; 				// Surface temperature algorithm option
    getline(snowFile, inLine, '\n');			// Read the remainder of the line
    snowFile >> mQgOption;  					// ground heat input
    getline(snowFile, inLine, '\n');			// Read the remainder of the line
    snowFile.close();

    ifstream snowparamFile(pFile.c_str());	// unit 88
    if (!snowparamFile.is_open()) {
        cerr << "Failed to open " << pFile << '\n';
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < Npar; i++) {
        snowparamFile >> snowparam[i];
        getline(snowparamFile, inLine, '\n');			// Read the remainder of the line
    }
    snowparamFile.close();

    bcparm(dtbar, bca, bcc, bcFile);	// warning we wont use the dtbar values we read here but use this read to get bca and bcc
    // dtbar values used are read in hyData and averaged for nearby gages
    ifstream dcFile(dfcFile.c_str());	// unit 88
    if (!dcFile.is_open()) {
        cerr << "Failed to open " << dfcFile << '\n';
        exit(EXIT_FAILURE);
    }
    i = 0;
    getline(dcFile, inLine, '\n');			// Read the first line.
    while (inLine.length() > 1) {
        line.str(inLine);
        line >> a >> b;
        i++;
        getline(dcFile, inLine, '\n');
    }
    ndepletionpoints = i;
    dfc = new double*[ndepletionpoints];
    for (j = 0; j < ndepletionpoints; j++) {
        dfc[j] = new double[2];
    }
    // rewind
    dcFile.clear();
    dcFile.seekg(0);

    for (i = 0; i < ndepletionpoints; i++) {
        dcFile >> dfc[i][0] >> dfc[i][1];
    }
    dcFile.close();

    for (j = 0; j < maxSlp; j++) {
        for (i = 0; i < Nxv; i++) {
            snowstatev[j][i] = 0.0;
        }
    }
    //	month,day,year,hour,
    timestep = 24;

    snowcontrol[0] = irad;
    snowcontrol[1] = ipflag;
    snowcontrol[2] = 10;
    snowcontrol[3] = nintstep;

    snowcontrol[6] = isurftmpoption;

    nstepday = 24/timestep*nintstep;
    vector<vector<double> > snowsurfacetemp(maxSlp,vector<double>(nstepday));   // maxSlp columns
    vector<vector<double> > snowaveragetemp(maxSlp,vector<double>(nstepday));   // maxSlp columns
    for (j = 0; j < maxSlp; ++j) {
        for (i = 0; i < nstepday; ++i) {
            snowsurfacetemp[j][i] = -9999.0;
            snowaveragetemp[j][i] = -9999.0;
        }
    }

    for (j = 0; j < maxSlp; j++) {
        snowsurfacetemp[j][nstepday-1] = 0.0;
        snowaveragetemp[j][nstepday-1] = 0.0;
    }

    if (snowcontrol[1] >= 1) {
        snowcontrol3_File.open(outFileName.c_str());
        if (!snowcontrol3_File.is_open()) {
            cerr << "Failed to open " << outFileName << '\n';
            exit(EXIT_FAILURE);
        }
    }

    //   initialize variables for mass balance
    for (i = 0; i < maxSlp; i++) {
        w1[i] = snowstatev[i][1];
        cump[i]    = 0.0;
        cume[i]    = 0.0;
        cummr[i]   = 0.0;
        errmbal[i] = 0.0;
    }

    // ***********************************************************************

    // SUBCATCHMENT SECTION

    // Landcare
    natball = 0;
    // implementing multiple raingauges for each sub-basin
    // (see also top.f, mddatav4.f)
    for (js = 0; js < Nsub; js++) {		// for each subbasin
        // Landcare
        natball += Nka[js];
    }		// Now pass 'bRain' instead of 'rain' and use row js of bRain, rather than row links(2,js) of rain
    //	do 3551 it=1,m
    // 3551	write(21,*)(bRain(js,it),js=1,Nsub)

    // write subbasin output headers
    if ( modwrt ) {
        if (idebugoutput >= 1) {
            lunmodFile << nBout << " " << m << '\n';
            lunmodFile << " Basin";
            lunmodFile << " TimeStep IrrDrainCat Afrac SWInput_mm Qlat_mm Qtot_mm Qb_mm";
            lunmodFile << " Recharge_mm SatEx_mm InfEx_mm SurfRo_mm SatAfrac InfAfrac";
            lunmodFile << " IntStore_mm WTDepth_mm SoilStore_mm Pet_mm Aet_mm";
            lunmodFile << " Irrig_mm GWTake_mm IrrDem_mm Prec_mm SWE_mm Sublim_mm";
            lunmodFile << " Tave_C Tdew_C Trange_C ErrClosure_mm\n";

            // Landcare
            if (mpe == -1) {
                lundatFile << Nsub << '\n';
            }
            luntopFile << "Measured and modeled flows\n";
            luntopFile << "Timestep (column 1), Measured reaches (columns 2 to " << dec << setw(3) << Neq+1;
            luntopFile << ") Modeled reaches (columns " << dec << setw(4) << Neq+2 << " to " << dec << setw(4) << Neq+Nout+1 << ")\n";
            luntopFile << "units ARE um/interval normalized by each basins own area";
            luntopFile << " unless they have a -ve site No. in which case they are lake";
            luntopFile << " levels in metres\n";
            luntopFile << "The next two rows give the number of flows to be used for fitting";
            luntopFile << " and printing, followed by the reach numbers to which they relate\n";
        }
    }
    isnow_method = 2;
    if (isnow_method != 2) {
        // get any snow input parameters
        ddf = -1.0;
        ifstream snowinpFile("snowinp.txt");
        if (!snowinpFile.is_open()) {
            cerr << "Failed to open 'snowinp.txt'\n";
        } else {
            snowinpFile >> ddf;
            snowinpFile.close();
        }
    } else {
        cout << "Snow input parameters are not used in this version of TopNet (isnow_method == 2)\n";
    }

    ofstream snowoutFile("snowout.txt");	// unit 78

    // ***********************************************************************

    // INITIALISE FOR REACH ROUTING

    // The routing cannot handle 0 slopes so fix those.
    // The relationship used is
    //  S = C A^theta = .133351 (A[m^2])^(-.425) = 47.31513 (A[mm^2])^(-.425)
    //  This was fit as a "lower bound" to the scatter in a slope vs area
    //  plot for the Grey river, New Zealand.

    for (jr1 = 1; jr1 <= model1::Nrch; jr1++) {
        jr = ll[jr1+ngut-1];
        // smin=47.31513 * area(jr)**(-0.425)    Area not valid due to reachlogic above inoperable
        Rp[0][jr-1] = max(smin, Rp[0][jr-1]);
    }
    for (i = 0; i < maxSlp; i++) {
        snowst[i] = 0.0;
    }
#ifdef ZBAR_IN
    double t0 = (double)clock()/(double)CLOCKS_PER_SEC;
    char cr;
    ifstream zbarInFile("zbar_in.dat");
    if (!zbarInFile) {
        cerr << "Failed to open zbar_in.dat\n";
        exit(1);
    }
    int in_istep, in_basin;
    for (istep = 0; istep <= m; istep++) {
        for (jsub = 0; jsub < Nsub; jsub++) {
            zbarInFile >> in_istep >> in_basin; // discard the first two integers.
            for (n = 0; n < nreg; n++) {
                zbarInFile >> zbar_in[istep][jsub][n];
            }
            zbarInFile.get(cr);	// read line end
        }
    }
    zbarInFile.close();
    double t1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    cout << t1 - t0 << " seconds to read depth to water file \n";
#endif
    if (nooksack) {
        n_irrig = 1;   // DGT warning.  If this is changed the logic associated with
        //                averaging depth_irrig_dem will need to be adjusted
        n_drainage = 2;
    }
    else {
        n_irrig    = 0;
        n_drainage = 0;
    }

    // start of time loop +++++++++++++++++++++++++++++++++++++++++
    cout << "Starting time loop\n";
    // cout << "Period starting date/time for calculation of mean is: " << asctime(startAnnual);
    for (istep = 0; istep <= m; istep++) {
#if TRACE
        static int nsteps = 0;
        if (nsteps < MAX_TRACE) {
            traceFile << setw(10) << "timestep " << istep << '\n';
        }
        nsteps++;
#endif
        if (timeinfo->tm_mday == 1) {
            strftime(dateStr, 11,"%Y %m %d", timeinfo);
            cout << "--------------------------- " << dec << setw(5) << istep << " --- " << dateStr << "-----------------------------\n";
        }
        //   LOOP OVER SUBBASINS
        // Work through basins in "stream order" order, i.e.,
        // do basins that feed a first order channels before second order and so on.
        // The order is already defined by array LL, and linkR(2,:) gives the basin number.
        // The first ngut rows in linkR refer to basins (basin # are transformed by
        // having ngut added to them). The next nrch rows of linkR tell which reach
        // is fed by which basin and/or upstream reach. We want the basin numbers
        // from column 2, rows ngut+1 to ngut+nrch in linkR and we want them in the order they
        // should be processed in. The order is in LL(ngut+1) to LL(ngut+nrch). DGT
        // introduced an additive factor of MAXCHN to avoid confusion between basin
        // and reach numbers and we have to remove it.

#ifdef SS
    if (timeinfo->tm_mon == 0 && timeinfo->tm_mday == 1) {   // January 1st
        cout << " Updating depth to water table.  timestep " << setw(5) << istep << " " << asctime(timeinfo);
        char cr;
        ifstream zbarInFile("zbar_ss.dat");
        if (!zbarInFile) {
            cerr << "Failed to open zbar_ss.dat\n";
            exit(1);
        }
        double MF_depth;    // MODFLOW output
        int sub;
        for (jsub = 0; jsub < Nsub; jsub++) {
            zbarInFile >> sub >> MF_depth;
            //cerr << dec << setw(3) << sub;
            for (n = 0; n < nreg; n++) {
                zbar[jsub][n] = MF_depth;
                //cerr << fixed << setw(12) << setprecision(6) << zbar[jsub][n];
            }
            zbarInFile.get(cr);	// read line end
            //cerr << '\n';
        }
        zbarInFile.close();
    }
#endif // SS
#ifdef IRR
    if (timeinfo->tm_mon == 8 && timeinfo->tm_mday == 30) {   // September 30th
        cout << " Updating depth to water table.  timestep " << setw(5) << istep << " " << asctime(timeinfo);
        char cr;
        ifstream zbarInFile("zbar_irr.dat");
        if (!zbarInFile) {
            cerr << "Failed to open zbar_irr.dat\n";
            exit(1);
        }
        double MF_depth;    // MODFLOW output
        int sub;
        for (jsub = 0; jsub < Nsub; jsub++) {
            zbarInFile >> sub >> MF_depth;
            //cerr << dec << setw(3) << sub;
            for (n = 0; n < nreg; n++) {
                zbar[jsub][n] = MF_depth;
                //cerr << fixed << setw(12) << setprecision(6) << zbar[jsub][n];
            }
            zbarInFile.get(cr);	// read line end
            //cerr << '\n';
        }
        zbarInFile.close();
    }
#endif // IRR
#ifdef NIRR
    if (timeinfo->tm_mon == 2 && timeinfo->tm_mday == 31) {   // March 31st
        cout << " Updating depth to water table.  timestep " << setw(5) << istep << " " << asctime(timeinfo);
        char cr;
        ifstream zbarInFile("zbar_nirr.dat");
        if (!zbarInFile) {
            cerr << "Failed to open zbar_nirr.dat\n";
            exit(1);
        }
        double MF_depth;    // MODFLOW output
        int sub;
        for (jsub = 0; jsub < Nsub; jsub++) {
            zbarInFile >> sub >> MF_depth;
            //cerr << dec << setw(3) << sub;
            for (n = 0; n < nreg; n++) {
                zbar[jsub][n] = MF_depth;
                //cerr << fixed << setw(12) << setprecision(6) << zbar[jsub][n];
            }
            zbarInFile.get(cr);	// read line end
            //cerr << '\n';
        }
        zbarInFile.close();
    }
#endif // NIRR

        for (jsub = 1; jsub <= Nsub; jsub++) {
            js = jsub-1;    // Fortran to C indexing

            prec = bRain[js][max(istep, 1)-1];
            jr = ll[js];

            // snow
            if (istep > 0) {

                //        Subroutine to compute ET  DGT 17 May 1998
                // parameters for ET - because at least albedo may be fitted, it is not possible
                // to put this loop outside calcts - therefore leave as is for the time being RPI 18/3/2004
                // debugging.  To debug a particular watershed uncomment the lines below and enter its topnetID
                //      if (jsub == 87 .and. istep == 245)then
                //	  js=jsub

                //	snowcontrol(1)=1
                //	else
                //	snowcontrol(1)=0
                //	endif
                albedo = Sp[11][js];
                rlapse = Sp[12][js];
                elevsb = Sp[13][js];
                temper_temp = temper[istep-1];
                dewp_temp = dewp[istep-1];
                trange_temp = tRange[istep-1];
                if (ievap_method != 0)
                    dewp_temp = bTdew[js][istep-1]; // for use with penman-Monteith
                if (ievap_method != 0)
                    trange_temp = min(30.0, max(0.0, bTmax[js][istep-1] - bTmin[js][istep-1])); // check for bad data
                if (ievap_method != 0)
                    temper_temp = (bTmax[js][istep-1] + bTmin[js][istep-1])/2;
                for (i = 0; i < 12; i++) {
                    for (j = 0; j < maxSlp; j++) {
                        bdtBarR[j][i] = bdtBar[i][j];
                    }
                }
                sHour = 0;
                //  Warning here - this code would have to be changed for time steps other than daily
                //  In general it would be better to inherit sHour from the calling program but coming in at 240000
                //  the integration of radiation across a day fails.  A more general solution to this issue would involve
                //  reprogramming hyri to handle time steps that cross the day break.
                //  Setting sHour=0 also achieves compatibility with the convention that inputs are associated with
                //  measurements recorded at the end of the time step (240000 for daily) but ET and snow computations
                //  integrate from the start time over the time step.
                elev = elevsb; // we are driving this using a basin average temperature, so it must be at basin average elev
                etall(bXlat[js], bXlon[js], stdlon, elev, bdtBarR[js], PETsngl, temper_temp, dewp_temp,
                      trange_temp, elevsb, albedo, rlapse, sDate, sHour, interval, m, istep, iyear, month, iday, ihr,
                      imm, isec, hour1, bTmin[js][istep-1], bTmax[js][istep-1], wind2m[istep-1], ievap_method);
                pet = PETsngl;
                ta= (bTmin[js][istep-1] + bTmax[js][istep-1])/2.0;
                p = bRain[js][istep-1]*3600.0/interval/1000.0; //mm/int *3600s/h / (s/int) / (1000mm/m) = m/hr for snowueb
                ws = wind2m[istep-1];
                //		RH=1 !unknown for Nooksack, hope we don't need it!
                qsiobs = 0; // unknown for Nooksack, hope we don't need it!
                qnetob = 0; // unknown for Nooksack, hope we don't need it!
                if (isnow_method == 2) {
                    snowcontrol[4] = iyear*10000 + month*100 + iday;
                    ihh = hour1;
                    imm = (hour1-ihh)*60;
                    iss = hour1*3600-ihh*3600-imm*60;
                    snowcontrol[5] = ihh*10000 + imm*100*iss;
                    snowforcing[0] = ta;
                    snowforcing[1] = p;
                    snowforcing[2] = ws;
                    snowforcing[3] = dewp_temp;
                    //	snowforcing(4) = 237.3/(1/(log(RH)/17.27+Ta/(Ta+237.3))-1)  ! from http://williams.best.vwh.net/ftp/avsig/avform.txt
                    snowforcing[4] = bTmax[js][istep-1] - bTmin[js][istep-1];
                    snowforcing[5] = qsiobs;
                    snowforcing[6] = qnetob;
                    snowsitev[0] = 0;   //  Sp(39,js) !0. !forest cover   DGT 4/1/05
                    snowsitev[1] = Sp[13][js]; //elevsb
                    snowsitev[2] = bXlat[js]; //lat
                    snowsitev[3] = bXlon[js]; //lon
                    snowsitev[4] = stdlon;  //stdlong
                    snowsitev[5] = Sp[13][js]; //elevtg is assumed  =  elevsb
                    snowsitev[6] = Sp[12][js]; //rlapse
                    snowsitev[7] = 0.0; //slope
                    snowsitev[8] = 0.0; //azimuth
                    snowueb(istep,jsub,snowsitev, snowstatev[js], snowparam, ndepletionpoints, dfc, snowcontrol, bdtBarR[js],
                            snowforcing, snowsurfacetemp[js], snowaveragetemp[js], timestep, nstepday,
                            surfacewaterinput, snowevaporation,  // outputs (both in m/h)
                            areafractionsnow, js+1);   // added js so that within snow one knows which element one is in for debugging
                    cump[js]  = cump[js]  + p*timestep;
                    cume[js]  = cume[js]  + snowevaporation*timestep;
                    cummr[js] = cummr[js] + surfacewaterinput*timestep;
                    errmbal[js] = w1[js] + cump[js] - cummr[js] - cume[js] - snowstatev[js][1];
                    if (snowcontrol[1] >= 2) {
                        snowcontrol3_File << dec << setw(2) << js;
                        snowcontrol3_File << dec << setw(5) << istep;
                        snowcontrol3_File << dec << setw(5) << iyear;
                        snowcontrol3_File << dec << setw(3) << month;
                        snowcontrol3_File << dec << setw(3) << iday;
                        snowcontrol3_File << fixed << setw(5)  << setprecision(1) << hour1;
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << surfacewaterinput;
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << snowevaporation;
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << areafractionsnow;
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << snowstatev[js][1];
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << cump[js];
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << cume[js];
                        snowcontrol3_File << fixed << setw(18) << setprecision(9) << cummr[js] << '\n';	//swe
                    }
                } else {
                    if (ddf >= 0) {
                        snow(snowoutFile, temper, elevtg[0], elevsb, rlapse, prec, ddf, snowst[js], interval, Nsub, m, js+1, istep, maxSlp, maxInt);
                    }
                }
                if ( modwrt && istep == m ) {
                    if (idebugoutput >= 1) {  // DGT 8/17/05 debugout
                        lunpFile << "\n\n Output for sub-catchment " << dec << setw(3) << js+1 << '\n';
                        lunpFile << " ---------------------------\n";
                    }
                }
            }

            prec = surfacewaterinput*1000.0*interval/3600.0;   //mm/timestep=m/h*1000mm/m*h/timestep

            wt0 = 1.0;
            kk = 0;
            depth_irrig_dem          = 0.0;
            //	xIRR(:)=0
            baseflow[js]           = 0.0;
            totalrunoff[js]        = 0.0;
            evap_for_watermgmt[js] = 0.0;
            potentialevap[js]      = 0.0;   // DGT 10/21/12 initialize for averaging over categories
            surfro[js]             = 0.0;
            canstore[js]           = 0.0;
            soilstore[js]          = 0.0;
            tiled[js]              = 0.0;
            ditchd[js]             = 0.0;

            zbm_d            = 0;
            acsem_d          = 0;
            s1_d             = 0;
            zbar_d           = 0.0;
            sr_d             = 0;
            cv_d             = 0;
            bal_d            = 0;
            sumr_d           = 0;
            sumq_d           = 0;
            sumae_d          = 0;
            s0_d             = 0;
            sumpe_d          = 0;
            qinst_out_d      = 0;
            dr_out_d         = 0;		// Initializing
            art_drainage_out = 0.0;		// Artificial drainage

#ifdef ZBAR_OUT
            if (istep > 0) {
                zbarFile << dec << setw(4) << istep;
                zbarFile << dec << setw(4) << jsub;
                // all nreg regions have the same value at this point in the program
                dateZbarFile << dec << setw(4) << istep;
                dateZbarFile << dec << setw(4) << jsub;
                dateZbarFile << dec << setw(11) << dateStr;
            }
#endif

            // the previous timestep's call to watermgmt calculated groundwater_to_take
            if (istep > 1) { //can't do any pumping on first timestep because we haven't called watermgmt yet: assume zero pumping when istep=1
                dth1 = Sp[3][js];  // dgt 11/4/07 added this line to get dth1 from corresponding basin parameter
                for (n = 0; n < nreg; n++) {
#ifdef ZBAR_IN
                    zbar[js][n] = zbar_in[istep][js][n];
#endif // ZBAR_IN
                    zbar[js][n] += groundwater_to_take[js]/(Sp[0][js]/1.0e6)/dth1;     //RAW 18-Jul-2005 bug fix: Sp(1,JS)/1e6 was Sp(1,JS)*1e6
                }
                //  m/timestep = m3/timestep/(m^2)/porosity
                //   DGT 11/4/07 changed the above from DTH to DTH1 because topmodel saturated zone works with that.
            }

            for (i_irrig = 1; i_irrig <= (n_irrig+1); i_irrig++) {
                if (i_irrig == 1) {
                    wt1 = wt0*(1.0 - Sp[19][js]); //unirrigated fraction
                    rate_irrig = 0;
                } else {
                    wt1 = wt0*Sp[19][js]; // irrigated fraction
                    if (istep == 0 || wt1 <= 0.0) {
                        rate_irrig = 0;
                    } else {
                        //use volume_irrig_sup[js] calculated in previous timestep
                        //                       mm^3        /      mm^2      / (sec) = mm/s
                        //   DGT 8/18/05.  Commented out the /float(interval).  The units going to topmod need to be mm/ts, the same as precipitation
                        //   DGT 8/18/05.                 mm^3                 /      mm^2 = mm/ts
                        rate_irrig = volume_irrig_sup[js]*1.0e9/(wt1*Sp[0][js]);
                        //   /(float(interval))
                    }
                }
                for (i_drainage = 1; i_drainage <= (n_drainage+1); i_drainage++) {
                    kp = (i_irrig - 1)*3 + i_drainage;   // DGT 11/3/07
                    // kp is a variable to index the combinations of irrigation and drainage to compare with kcase and control
                    // detail output.  There are 3 drainage cases.  The first the associated with i_irrig will be numbered 1,2,3
                    // The next three will be numbered 4, 5, 6 and so on.
                    if (i_drainage == 1) { //Naturally drained
                        wt2 = 1.0 - Sp[15][js] - Sp[16][js]; //16=tilefraction, 17=ditchfraction
                        art_drainage = 0;
                    } else if (i_drainage == 2) {     //Tile drained
                        wt2 = Sp[15][js]; //16=tilefraction
                        art_drainage = Sp[17][js]; //watch for units
                    } else if (i_drainage == 3) {      //Ditch drained
                        wt2 = Sp[16][js]; //17=ditchfraction
                        art_drainage = Sp[18][js];
                    }
                    wt12 = wt1*wt2; //wt12 is fraction of basin covered by this drainage-irrigation combo
                    kk++;
                    if (wt12 > 0) {
                        for ( i = 0; i < MAX_NTDH; ++i ) {
                            dr1[i] = dr[js][kk-1][i];
                            qinst1[i] = qinst[js][kk-1][i];
                        }
                        topmod(Si, Sp, jsub, Nka, tl[js], atb, pka, nd, cl, pd, units, irr_l, modwrt,
                               ipsub, ipatb, stim, prec, pet, interval, art_drainage, rate_irrig, month,
                               m, mps, mpe, qinst_out_0,  dr_out_0, ndump, ntdh, istep, maxC, zbm[js][kk-1],
                               maxA, maxSlp, maxInt, sumr[js][kk-1], sumq[js][kk-1], sumae[js][kk-1], s0[js][kk-1], q0[js],
                               sr[js][kk-1], cv[js][kk-1], aciem[js][kk-1], acsem[js][kk-1], sumpe[js][kk-1], sumie[js][kk-1],
                               sumqb[js][kk-1], sumce[js][kk-1], sumsle[js][kk-1], sumr1[js][kk-1], qb[js], qinst1,
                               dr1, sumqv[js][kk-1], sumse[js][kk-1], zbar[js][kk-1], tdh, zr[js], ak0fzrdt[js],
                               logoqm[js], qvmin[js], dth[js], sumad, evap_mm, qlat_mm, ipflag, rirr, js,
                               upwelling_temporary, recharge_temporary, precip_minus_et_temporary);

                        upwelling_temporary *= wt12;
                        recharge_temporary  *= wt12;
                        precip_minus_et_temporary  *= wt12;

                        if (i_irrig == 1 && i_drainage == 1)
                            unirrigated_natural_upwelling[js] = upwelling_temporary;
                        if (i_irrig == 1 && i_drainage == 2)
                            unirrigated_tile_upwelling[js]    = upwelling_temporary;
                        if (i_irrig == 1 && i_drainage == 3)
                            unirrigated_ditch_upwelling[js]   = upwelling_temporary;
                        if (i_irrig == 2 && i_drainage == 1)
                            irrigated_natural_upwelling[js]   = upwelling_temporary;
                        if (i_irrig == 2 && i_drainage == 2)
                            irrigated_tile_upwelling[js]      = upwelling_temporary;
                        if (i_irrig == 2 && i_drainage == 3)
                            irrigated_ditch_upwelling[js]     = upwelling_temporary;

                        if (i_irrig == 1 && i_drainage == 1)
                            unirrigated_natural_recharge[js]  = recharge_temporary;
                        if (i_irrig == 1 && i_drainage == 2)
                            unirrigated_tile_recharge[js]     = recharge_temporary;
                        if (i_irrig == 1 && i_drainage == 3)
                            unirrigated_ditch_recharge[js]    = recharge_temporary;
                        if (i_irrig == 2 && i_drainage == 1)
                            irrigated_natural_recharge[js]    = recharge_temporary;
                        if (i_irrig == 2 && i_drainage == 2)
                            irrigated_tile_recharge[js]       = recharge_temporary;
                        if (i_irrig == 2 && i_drainage == 3)
                            irrigated_ditch_recharge[js]      = recharge_temporary;

                        upwelling[js] += upwelling_temporary;   // aggregate over i_irrig and i_drainage
                        recharge[js]  += recharge_temporary;    // aggregate over i_irrig and i_drainage
                        precip_minus_et[js] += precip_minus_et_temporary;
                        annual_upwelling[js]  += upwelling_temporary;
                        annual_recharge[js]   +=  recharge_temporary;
                        monthly_upwelling[js] += upwelling_temporary;
                        monthly_recharge[js]  +=  recharge_temporary;

                        for ( i = 0; i < MAX_NTDH; ++i ) {
                            dr[js][kk-1][i] = dr1[i];
                            qinst[js][kk-1][i] = qinst1[i];
                        }
                        //  DGT 11/2/07 added qlat and ipflag.  ipflag for debugging.  qlat to return lateral outflows without
                        //     overland flow delays to facilitate mass balance checks
                        //  Added subscript (JS) to Q0 in the calling statement above so that Q0 differences
                        //  between subbasins are preserved
                        //  Added sumad to retrieve artificial drainage calcs

                        if (i_irrig > 1) {
                            irrigation(sr[js][kk-1], Sp, js+1, interval, maxSlp, dep); //watch for units!!!
                            depth_irrig_dem = depth_irrig_dem + wt2*dep; //dep is in m/timestep
                            // DGT 8/20/05 changed the weight in the above  from wt12 to wt2 because depth only should be weighted
                            // among the different drainage classes within irrigated area
                        } else {
                            dep = 0.0;
                        }
                        zbm_d                  += wt12*zbm[js][kk-1];
                        acsem_d                += wt12*acsem[js][kk-1];
                        zbar_d                 += wt12*zbar[js][kk-1];
                        sr_d                   += wt12*sr[js][kk-1];
                        cv_d                   += wt12*cv[js][kk-1];
                        sumr_d                 += wt12*sumr[js][kk-1];
                        sumq_d                 += wt12*sumq[js][kk-1];
                        sumae_d                += wt12*sumae[js][kk-1];
                        sumpe_d                += wt12*sumpe[js][kk-1];
                        s0_d                   += wt12*s0[js][kk-1];
                        qinst_out_d            += wt12*qinst_out_0;
                        dr_out_d               += wt12*dr_out_0;    // 6/10/05  DGT keeping track of total runoff
                        art_drainage_out       += wt12*sumad;       // 6/28/05   DGT Artificial drainage
                        evap_for_watermgmt[js] += wt12*evap_mm;     // DGT 8/17/05 keeping track of evap
                        potentialevap[js]      += wt12*rirr[12];    // DGT 10/21/12  keeping track of pet
                        surfro[js]             += wt12*rirr[6]/1000.0*(Sp[0][js]/1.0e6)/interval;
                        canstore[js]           += wt12*rirr[9];
                        soilstore[js]          += wt12*rirr[11];
                        annual_potentialevap[js]  += wt12*rirr[12];
                        monthly_potentialevap[js] += wt12*rirr[12];
                        annual_evaporation[js]    += wt12*evap_mm;
                        monthly_evaporation[js]   += wt12*evap_mm;

                        if (i_drainage  == 2) {
                            tiled[js] += wt12*sumad*1.0e-9; // sumad is mm^3/s - now in m^3/s
                        }
                        if (i_drainage == 3) {
                            ditchd[js] += wt12*sumad*1.0e-9;
                        }

                        //  Detail topsbd writes
                        if (idebugoutput >= 1) {
                            if (modwrt && iBout[js] == 1) {
                                if (istep >= mi && istep > 0) {
                                    if (js+1 == idebugbasin || idebugbasin == 0) {
                                        if (kp == idebugcase || idebugcase == 0) {
                                            lunmodFile << dec << setw(5) << js+1 << dec << setw(8) << istep;
                                            lunmodFile << " " << dec << setw(3) << kp;
                                            lunmodFile << fixed << setw(11) << setprecision(7) << wt12;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << prec;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << qlat_mm;
                                            for (jj = 1; jj <= 13; jj++) {
                                                lunmodFile << " " << fixed << setw(11) << setprecision(6) << rirr[jj];
                                            }
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << rate_irrig;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << groundwater_to_take[js]/(Sp[0][js]/1.0e6)*1000.0;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << dep*1000.0;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << bRain[js][istep-1];
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << snowstatev[js][1]*1000.0;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << snowevaporation*timestep*1000.0;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << (bTmax[js][istep-1] + bTmin[js][istep-1])/2.0;
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << bTdew[js][istep-1];
                                            lunmodFile << " " << fixed << setw(11) << setprecision(6) << bTmax[js][istep-1] - bTmin[js][istep-1];
                                            lunmodFile << " " << fixed << setw(14) << setprecision(8) << rirr[14] << '\n';
                                        }
                                    }
                                }
                            }
                        }
                    }
                } // i_drainage
            } // i_irrig

            //  The one line below is a substantive change DGT 10/13/12.  It amounts to an assumption of
            //   lumped depth to water table, rather than separate depth to water table for each drainage and
            //   irrigation component.  It was made to fix the issue of groundwater levels declining indefinitely
            //   due to there being no recharge from artificially drained areas
            for (n = 0; n < nreg; n++) {
                zbar[js][n] = zbar_d;
#ifdef ZBAR_OUT
                if (istep > 0) {
                    zbarFile << scientific << setw(17) << setprecision(9) << zbar[js][n]; // all nreg regions now have been set to a single value
                }
#endif
            }
#ifdef ZBAR_OUT
            if (istep > 0) {
                zbarFile << '\n';
                dateZbarFile << scientific << setw(17) << setprecision(9) << zbar[js][0];   // Record only the first region.
                dateZbarFile << '\n';
            }
#endif
            if (istep > 0)
                tempave[js] = (bTmax[js][istep-1] + bTmin[js][istep-1])/2.0; // DGT 10/21/12 record ave temperature for output

            //accumulate the fluxes from each part of the sub-basin as m3/timestep
            baseflow[js] = qinst_out_d*interval*1.0e-9; // m^3/ts = mm^3/s *s/ts *m3/mm3

            //   Use Total Runoff from the floating point DR variable rather than the integer IRR topmodel output
            //    to avoid some divide by zero's later on in water management
            totalrunoff[js] = dr_out_d*interval*1.0e-9; // m^3/ts = mm^3/s *s/ts *m3/mm3
            ArtDrainage[js] = art_drainage_out*interval*1.0e-9 ;  // DGT 6/28/05  Keep in same units as totalrunoff
            annual_ArtDrainage[js]  += ArtDrainage[js];
            monthly_ArtDrainage[js] += ArtDrainage[js];

            if (modwrt && istep == m) {
                if (idebugoutput >= 1) {  // DGT 8/17/05 debugout
                    if (modwrt) {
                        lunpFile << "MIN ZBAR=";
                        lunpFile << fixed << setw(22) << setprecision(17) << zbm_d << " MAX CONT AREA=" << acsem_d << '\n';
                    }
                    if (modwrt) {
                        lunpFile << "LAMBDA=";
                        lunpFile << fixed << setw(21) << setprecision(15) << tl[js] << '\n';
                    }
                }
                //  Storage at the end
                dth1 = Sp[3][js];
                s1_d = - zbar_d*dth1 + sr_d + cv_d;
                bal_d = sumr_d - sumq_d - sumae_d + (s0_d - s1_d);
                if (idebugoutput >= 1) { // dgt 8/17/05 debugout
                    lunpFile << "water balance: bal= " << bal_d << " sumr= " << sumr_d << '\n';
                }
                if ( fabs(bal_d) > (sumr_d + sumq_d)*1.0e-3 ) {
                    // can't write to lunco1, so put it in toperror.txt
                    //			write(21,*)"water balance: bal=",bal_d," sumr=",sumr_d,
                    //    +					" sumq=",sumq_d," it=",istep
                }
                if (idebugoutput >= 1) {  // DGT 8/17/05 debugout
                    lunpFile << '\n';
                    lunpFile << " Rainfall    =" << fixed << setw(9) << setprecision(1) << sumr_d/units;
                    lunpFile << " Total Evap  =" << fixed << setw(9) << setprecision(1) << sumae_d/units << '\n';
                    lunpFile << " Model Runoff=" << fixed << setw(9) << setprecision(1) << sumq_d/units;
                    lunpFile << " Total PET   =" << fixed << setw(9) << setprecision(1) << sumpe_d/units << '\n';
                    lunpFile << " Incr.Storage=" << fixed << setw(9) << setprecision(1) << (s1_d - s0_d)/units << '\n';
                }
            } // finish

            vol_irrig_demand[js] = depth_irrig_dem;  // *(Sp(1,JS)/1e6) !m3/timestep = m/timestep * m^2

            if ( modwrt && istep == m ) {
                if (idebugoutput >= 1) {  // DGT 8/17/05 debugout
                    lunpFile << " The parameters are\n";
                    for (i = 1; i <= Nsp; i++) {
                        lunpFile << " " << scientific << setw(15) << setprecision(6) << Sp[i-1][js] << '\n';
                    }
                    //lunpFile << '\n';
                    lunpFile << " Initial conditions\n";
                    for (i = 1; i <= Nsi; i++) {
                        lunpFile << " " << scientific << setw(15) << setprecision(6) << Si[i-1][js] << '\n';
                    }
                    lunpFile << '\n';
                }
            }

            //     SET UP THE START TIME OF THE OUTPUT TIME SERIES OF EACH GUTTER
        } // subbasin_loop

        // THAT IS THE END OF THE SUBCATCHMENTS

        // now we know runoff and soil moisture, so we can do a water managment step

        if (istep > 0) {
            for (jsub = 0; jsub < Nsub; jsub++) {
                precip_for_watermgmt[jsub] = bRain[jsub][istep-1]; //mm/timestep
                annual_precipitation[jsub]  += bRain[jsub][istep-1];
                monthly_precipitation[jsub] += bRain[jsub][istep-1];
            }
        }

        scalefactor = 1.0;
        strftime(dateStr, 11,"%Y %m %d", timeinfo);
        if (istep > 0) {
            Write_Line_valarray(extraFiles[0], extraFileNames[0], dateStr, irrigated_natural_upwelling,   Nsub, scalefactor);
            Write_Line_valarray(extraFiles[1], extraFileNames[1], dateStr, irrigated_tile_upwelling,      Nsub, scalefactor);
            Write_Line_valarray(extraFiles[2], extraFileNames[2], dateStr, irrigated_ditch_upwelling,     Nsub, scalefactor);
            Write_Line_valarray(extraFiles[3], extraFileNames[3], dateStr, unirrigated_natural_upwelling, Nsub, scalefactor);
            Write_Line_valarray(extraFiles[4], extraFileNames[4], dateStr, unirrigated_tile_upwelling,    Nsub, scalefactor);
            Write_Line_valarray(extraFiles[5], extraFileNames[5], dateStr, unirrigated_ditch_upwelling,   Nsub, scalefactor);
        }
        irrigated_natural_upwelling   = 0.0;
        irrigated_tile_upwelling      = 0.0;
        irrigated_ditch_upwelling     = 0.0;
        unirrigated_natural_upwelling = 0.0;
        unirrigated_tile_upwelling    = 0.0;
        unirrigated_ditch_upwelling   = 0.0;

        if (istep > 0) {
            Write_Line_valarray(extraFiles[6],  extraFileNames[6],  dateStr, irrigated_natural_recharge,   Nsub, scalefactor);
            Write_Line_valarray(extraFiles[7],  extraFileNames[7],  dateStr, irrigated_tile_recharge,      Nsub, scalefactor);
            Write_Line_valarray(extraFiles[8],  extraFileNames[8],  dateStr, irrigated_ditch_recharge,     Nsub, scalefactor);
            Write_Line_valarray(extraFiles[9],  extraFileNames[9],  dateStr, unirrigated_natural_recharge, Nsub, scalefactor);
            Write_Line_valarray(extraFiles[10], extraFileNames[10], dateStr, unirrigated_tile_recharge,    Nsub, scalefactor);
            Write_Line_valarray(extraFiles[11], extraFileNames[11], dateStr, unirrigated_ditch_recharge,   Nsub, scalefactor);
        }
        irrigated_natural_recharge   = 0.0;
        irrigated_tile_recharge      = 0.0;
        irrigated_ditch_recharge     = 0.0;
        unirrigated_natural_recharge = 0.0;
        unirrigated_tile_recharge    = 0.0;
        unirrigated_ditch_recharge   = 0.0;


        Write_OutputLine_valarray(oFile[15], "results/upwelling_mm.txt", istep, dateStr, upwelling, Nsub, scalefactor);
        Write_Line_valarray(oFile[16],  "results/recharge_mm.txt",  dateStr, recharge,   Nsub, scalefactor);
        Write_Line_valarray(extraFiles[12],  extraFileNames[12],  dateStr, precip_minus_et,   Nsub, scalefactor);

        upwelling = 0.0;
        recharge  = 0.0;
        precip_minus_et = 0.0;

        if (timeinfo->tm_mon == 8 && timeinfo->tm_mday == 30 && istep > 0) {    // start of the next water year
            daysInYear = difftime(mktime(timeinfo),startAnnual)/86400.0;
            if (daysInYear >= 365.0) {
                scalefactor = 1.0;
                for (jsub = 0; jsub < Nsub; jsub++) {
                    annAve_upwelling[jsub]     += annual_upwelling[jsub];
                    annAve_recharge[jsub]      += annual_recharge[jsub];
                    annAve_precipitation[jsub] += annual_precipitation[jsub];
                    annAve_potentialevap[jsub] += annual_potentialevap[jsub];
                    annAve_evaporation[jsub]   += annual_evaporation[jsub];
                    annAve_ArtDrainage[jsub]   += annual_ArtDrainage[jsub];
                }
                countFullYears++;
            } else {
                scalefactor = 365.0/daysInYear;
            }
            cout << "daysInYear " << fixed << setw(5) << setprecision(0) << daysInYear;
            cout << fixed << setw(9) << setprecision(6) << " scalefactor " << scalefactor;
            cout << " timestep " << setw(5) << istep << " " << asctime(timeinfo);

            strftime(dateStr, 11,"%Y %m %d", timeinfo);
            Write_OutputTotal_valarray(oFile[18], resultsFileNames[18-offset], dateStr, annual_upwelling,     Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[19], resultsFileNames[19-offset], dateStr, annual_recharge,      Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[20], resultsFileNames[20-offset], dateStr, annual_precipitation, Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[21], resultsFileNames[21-offset], dateStr, annual_potentialevap, Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[36], resultsFileNames[36-offset], dateStr, annual_evaporation,   Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[33], resultsFileNames[33-offset], dateStr, annual_ArtDrainage,   Nsub, scalefactor);
            annual_upwelling      = 0.0; // re-initialize array
            annual_recharge       = 0.0; // re-initialize array
            annual_precipitation  = 0.0; // re-initialize array
            annual_potentialevap  = 0.0; // re-initialize array
            annual_evaporation    = 0.0; // re-initialize array
            annual_ArtDrainage    = 0.0; // re-initialize array
            startAnnual = mktime(timeinfo);
        }

        // ------------------------------------------------------------------------------
        if (timeinfo->tm_mday == 1 && istep > 0) {   // start of the next month
            scalefactor = 1.0;
            //cout << " timestep " << setw(5) << istep << " " << asctime(timeinfo);
            //cout << " timestep " << setw(5) << istep << " " << timeinfo->tm_zone << " " << asctime(timeinfo);
            strftime(dateStr, 11,"%Y %m %d", timeinfo);
            Write_OutputTotal_valarray(oFile[23], resultsFileNames[23-offset], dateStr, monthly_upwelling,     Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[24], resultsFileNames[24-offset], dateStr, monthly_recharge,      Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[25], resultsFileNames[25-offset], dateStr, monthly_precipitation, Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[26], resultsFileNames[26-offset], dateStr, monthly_potentialevap, Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[37], resultsFileNames[37-offset], dateStr, monthly_evaporation,   Nsub, scalefactor);
            Write_OutputTotal_valarray(oFile[34], resultsFileNames[34-offset], dateStr, monthly_ArtDrainage,   Nsub, scalefactor);
            monthly_upwelling      = 0.0; // re-initialize array
            monthly_recharge       = 0.0; // re-initialize array
            monthly_precipitation  = 0.0; // re-initialize array
            monthly_potentialevap  = 0.0; // re-initialize array
            monthly_evaporation    = 0.0; // re-initialize array
            monthly_ArtDrainage    = 0.0; // re-initialize array
        }
        // DGT 10/21/12  Additional writes for Christina
        scalefactor = 1.0;
        Write_OutputLine_valarray(oFile[0], "results/Potential_evapotranspiration_mm.txt", istep, dateStr, potentialevap, Nsub, scalefactor);
        Write_OutputLine_vector(oFile[1],   "results/TemperatureAve_C.txt",                istep, tempave,       Nsub, scalefactor);
        Write_OutputLine_vector(oFile[2],   "results/Surface_runoff_cms.txt",              istep, surfro,        Nsub, scalefactor);
        Write_OutputLine_vector(oFile[3],   "results/Canopy_storage_mm.txt",               istep, canstore,      Nsub, scalefactor);
        Write_OutputLine_vector(oFile[4],   "results/Soil_storage_mm.txt",                 istep, soilstore,     Nsub, scalefactor);

        scalefactor = 1000.0;
        double *temporary = new double[Nsub];
        for (j = 0; j < Nsub; j++) {
            temporary[j] = zbar[j][0];  // row 0; all elements of the row are the same at this point.
        }
        Write_OutputLine(oFile[5], "results/Depth_to_Water_mm.txt", istep, temporary, Nsub, scalefactor);
        delete [] temporary;

        scalefactor = 1.0;
        Write_OutputLine_vector(oFile[6], "results/Tile_drainage_cms.txt", istep, tiled, Nsub, scalefactor);
        Write_OutputLine_vector(oFile[7], "results/Ditch_drainage_cms.txt", istep, ditchd, Nsub, scalefactor);

        // Added ArtDrainage to arguments for output
        watermgmt(sDate, sHour, dateStr, istep, m, totalrunoff, baseflow, ArtDrainage, vol_irrig_demand, maxSlp,
                  evap_for_watermgmt, precip_for_watermgmt, volume_irrig_sup, groundwater_to_take);
        // ---------------------------------------------------------------------------------------------
        if (istep > 0) {
#ifdef DEBUG
            if (timeinfo->tm_mon == 8 && timeinfo->tm_mday == 30) {
                for (i = 0; i < NumUser; ++i) {
                    debugFile1 << "User[" << dec << setw(3) << i+1 << "].LinkUserToReturnflow[1] = ";
                    int j_return = input_structures::User[i].LinkUserToReturnflow[0];
                    debugFile1 << dec << setw(3) << j_return;
                    debugFile1 << " Link[" << j_return << "].USNode = " << other_structures::Link[j_return-1].USNode;
                    debugFile1 << " Link[" << j_return << "].DSNode = " << dec << setw(6) << other_structures::Link[j_return-1].DSNode;
                    debugFile1 << j_return << "  Link[" << j_return << "].Flow = ";
                    debugFile1 << fixed << setw(13) << setprecision(6) << other_structures::Link[j_return-1].Flow << '\n';
                }
            }
#endif
            // ---------------------------------------------------------------------------------------------
            // The code in the next two blocks must come after watermgmt() is called.
            returnflow = 0.0;   // initialize for this time step
            GroundWaterReturnFlow = 0.0;
            SurfaceWaterReturnFlow = 0.0;
            for (i = 0; i < NumUserSource; ++i) { // NumUsers > NumuserSourceReturn
                j = input_structures::User[sourceUserMap[i]].LinkUserToReturnflow[0];
                n = other_structures::UserSourceTable[i].DrainageID;
                if (other_structures::Link[j].LinkCode == constant_definitions::ReturnFlowLinkCode) {
                    if (other_structures::Node[other_structures::Link[j-1].DSNode-1].Type == constant_definitions::GroundwaterNodeCode) {
                        GroundWaterReturnFlow[n-1] += other_structures::Link[j-1].Flow;
                    } else {
                        SurfaceWaterReturnFlow[n-1] += other_structures::Link[j-1].Flow;
                    }
                }
                returnflow[n-1] += other_structures::Link[j-1].Flow;    // += because some drainages have more than one user.
                monthly_returnflow[n-1] += other_structures::Link[j-1].Flow;
                annual_returnflow[n-1]  += other_structures::Link[j-1].Flow;
            }
            // ---------------------------------------------------------------------------------------------
            for (int ncode = 0; ncode < NuserTypes; ++ncode) {
                for (i = 0; i < NumUserSource; i++) {
                    if (input_structures::User[sourceUserMap[i]].UsersType == userTypeCode[ncode]) {
                        n = other_structures::UserSourceTable[i].DrainageID;
                        BasinWithdrawalByUser[userTypeCode[ncode]-1][n-1] += input_structures::User[sourceUserMap[i]].Withdrawal[istep-1];
#ifdef DEBUG
                        if (flag3  && (n == 87)) {
                            debugFile3 << "UserType " << left << setw(32) << UserTypeNames[userTypeCode[ncode]-1] << "  ReturnFlowID ";
                            debugFile3 << dec << setw(3) << input_structures::User[sourceUserMap[i]].ReturnFlowID;
                            debugFile3 << " Drainage " << dec << setw(3) << n << " User " << dec << setw(3) << sourceUserMap[i];
                            debugFile3 << " SourceMixingID "  << dec << setw(3) <<  input_structures::User[sourceUserMap[i]].SourceMixingID  << " ";
                            debugFile3 << input_structures::User[sourceUserMap[i]].UserID << " User[" << sourceUserMap[i] << "].Withdrawal[" << istep-1 << "] "<<
                                 input_structures::User[sourceUserMap[i]].Withdrawal[istep-1] << endl;
                        }
#endif
                    }
                }
            }
#ifdef DEBUG
            flag3 = false;
#endif
            // ---------------------------------------------------------------------------------------------

            scalefactor = 1.0;
            if (timeinfo->tm_mday == 1) {   // start of the next month
                Write_OutputTotal_valarray(oFile[27], resultsFileNames[27-offset], dateStr, monthly_returnflow, Nsub, scalefactor);
                monthly_returnflow = 0.0;   // re-initialize the array
            }
            if (timeinfo->tm_mon == 8 && timeinfo->tm_mday == 30) { // end of water year
                Write_OutputTotal_valarray(oFile[22], resultsFileNames[22-offset], dateStr, annual_returnflow, Nsub, scalefactor);
                if (daysInYear >= 365.0) {
                    for (jsub = 0; jsub < Nsub; jsub++) {
                        annAve_returnflow[jsub] += annual_returnflow[jsub];
                    }
                }
                annual_returnflow = 0.0;   // re-initialize the array

                for (int ncode = 0; ncode < NuserTypes; ++ncode) {
                    scalefactor = 1.0;
                    Write_OutputTotal_vector(oFile[withdrawalOffset+ncode], drainageWithdrawalTotalsFileNames[ncode], dateStr, BasinWithdrawalByUser[ncode], Nsub, scalefactor);
                    if (daysInYear >= 365.0) {
                        for (jsub = 0; jsub < Nsub; jsub++) {
                            annAve_basin_withdrawal[ncode][jsub] += BasinWithdrawalByUser[ncode][jsub];
                        }
                    }
                    for (jsub = 0; jsub < Nsub; jsub++) {
                        BasinWithdrawalByUser[ncode][jsub] = 0.0;   // re-initialize the array
                    }
                }

            } // if end of water year
        }
        Write_OutputLine_valarray(oFile[17], "results/return_flow_by_drainage.txt", istep, dateStr, returnflow, Nsub, scalefactor);
        // -----------------------------------------------------------------------------------------------------
        if (istep > 0) {
            updatetime(iyear, month, iday, hour1, timestep);
        }
        timeinfo->tm_mday += 1;
        mktime(timeinfo);
        if (timeinfo->tm_hour == 1) {
            timeinfo->tm_hour -=1;
            mktime(timeinfo);
        }
        if (timeinfo->tm_hour == 23) {
            timeinfo->tm_hour +=1;
            mktime(timeinfo);
        }
    }	// time_loop

    // Final outputs
    scalefactor = 1.0;
    strftime(dateStr, 11,"%Y %m %d", timeinfo);
    //Write_Line_valarray(extraFiles[0], extraFileNames[0], dateStr, irrigated_natural_upwelling,   Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[1], extraFileNames[1], dateStr, irrigated_tile_upwelling,      Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[2], extraFileNames[2], dateStr, irrigated_ditch_upwelling,     Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[3], extraFileNames[3], dateStr, unirrigated_natural_upwelling, Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[4], extraFileNames[4], dateStr, unirrigated_tile_upwelling,    Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[5], extraFileNames[5], dateStr, unirrigated_ditch_upwelling,   Nsub, scalefactor);

    //Write_Line_valarray(extraFiles[6],  extraFileNames[6],  dateStr, irrigated_natural_recharge,   Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[7],  extraFileNames[7],  dateStr, irrigated_tile_recharge,      Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[8],  extraFileNames[8],  dateStr, irrigated_ditch_recharge,     Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[9],  extraFileNames[9],  dateStr, unirrigated_natural_recharge, Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[10], extraFileNames[10], dateStr, unirrigated_tile_recharge,    Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[11], extraFileNames[11], dateStr, unirrigated_ditch_recharge,   Nsub, scalefactor);

    // Final totals
    daysInYear = difftime(mktime(timeinfo),startAnnual)/86400.0;
    cout << "daysInYear " << fixed << setw(5) << setprecision(0) << daysInYear;
    if (daysInYear >= 365.0) {
        scalefactor = 1.0;
    } else {
        scalefactor = 365.0/daysInYear;
    }
    cout << fixed << setw(9) << setprecision(6) << " scalefactor " << scalefactor;
    //Write_OutputTotal_valarray(oFile[18], resultsFileNames[18-offset], dateStr, annual_upwelling,     Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[19], resultsFileNames[19-offset], dateStr, annual_recharge,      Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[20], resultsFileNames[20-offset], dateStr, annual_precipitation, Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[21], resultsFileNames[21-offset], dateStr, annual_potentialevap, Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[33], resultsFileNames[33-offset], dateStr, annual_ArtDrainage,   Nsub, scalefactor);
    for (int ncode = 0; ncode < NuserTypes; ++ncode) {
        Write_OutputTotal_vector(oFile[withdrawalOffset+ncode], drainageWithdrawalTotalsFileNames[ncode], dateStr, BasinWithdrawalByUser[ncode], Nsub, scalefactor);
    }
    scalefactor = 1.0;
    //Write_OutputTotal_valarray(oFile[23], resultsFileNames[23-offset], dateStr, monthly_upwelling,     Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[24], resultsFileNames[24-offset], dateStr, monthly_recharge,      Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[25], resultsFileNames[25-offset], dateStr, monthly_precipitation, Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[26], resultsFileNames[26-offset], dateStr, monthly_potentialevap, Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[34], resultsFileNames[34-offset], dateStr, monthly_ArtDrainage,   Nsub, scalefactor);
    cout << " timestep " << setw(5) << istep-1 << " " << asctime(timeinfo);

    // End of time loop  for reaches ***************************************************
    // Added ArtDrainage to arguments for output
    //final call to write/close output files
    watermgmt(sDate, sHour, dateStr, -1, m, totalrunoff, baseflow, ArtDrainage, vol_irrig_demand, maxSlp, //inputs, ignored this time
              evap_for_watermgmt, precip_for_watermgmt, volume_irrig_sup, groundwater_to_take);
    if (idebugoutput >= 1) {
        lunpFile << "Total snow mass balance error = "  << '\n';	// 'topinfo_v7.txt'
        for (js = 0; js < Nsub; ++js) {
            lunpFile << dec << setw(3) << js;
            lunpFile << scientific << setw(15) << setprecision(6) << errmbal[js] << '\n';
        }
    }
    // ---------------------------------------------------------------------------------------------
    // The code in this block must come after watermgmt() is called.
    returnflow = 0.0;   // initialize for this final time step
    for (i = 0; i < NumUserSource; ++i) { // NumUsers > NumuserSourceReturn
        j = input_structures::User[sourceUserMap[i]].LinkUserToReturnflow[0];
        n = other_structures::UserSourceTable[i].DrainageID;
        returnflow[n-1] += other_structures::Link[j-1].Flow;
        monthly_returnflow[n-1] += other_structures::Link[j-1].Flow;
        annual_returnflow[n-1]  += other_structures::Link[j-1].Flow;
    }

    scalefactor = 1.0;
    //Write_OutputTotal_valarray(oFile[27], resultsFileNames[27-offset], dateStr, monthly_returnflow, Nsub, scalefactor);
    //Write_OutputTotal_valarray(oFile[22], resultsFileNames[22-offset], dateStr, annual_returnflow, Nsub, scalefactor);
    Write_OutputLine_valarray(oFile[17], "results/return_flow_by_drainage.txt", istep, dateStr, returnflow, Nsub, scalefactor);
    scalefactor = 1.0/static_cast<double>(countFullYears); // converts sum of annual sums into mean of annual sums
    stringstream yearStr;
    yearStr << countFullYears;
    Write_OutputTotal_valarray(oFile[28], resultsFileNames[28-offset], yearStr.str(), annAve_upwelling,     Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[29], resultsFileNames[29-offset], yearStr.str(), annAve_recharge,      Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[30], resultsFileNames[30-offset], yearStr.str(), annAve_precipitation, Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[31], resultsFileNames[31-offset], yearStr.str(), annAve_potentialevap, Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[38], resultsFileNames[38-offset], yearStr.str(), annAve_evaporation,   Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[32], resultsFileNames[32-offset], yearStr.str(), annAve_returnflow,    Nsub, scalefactor);
    Write_OutputTotal_valarray(oFile[35], resultsFileNames[35-offset], yearStr.str(), annAve_ArtDrainage,   Nsub, scalefactor);
    for (int ncode = 0; ncode < NuserTypes; ++ncode) {
        Write_OutputTotal_vector(oFile[withdrawalOffset+ncode+NuserTypes], drainageWithdrawalAnnAveFileNames[ncode], yearStr.str(),
            annAve_basin_withdrawal[ncode], Nsub, scalefactor);
    }
    // -----------------------------------------------------------------------------------------------------
    istep = -1;     //  Close files on additional writes for Christina
    scalefactor = 1.0;
    Write_OutputLine_valarray(oFile[0],"results/Potential_evapotranspiration_mm.txt", istep, dateStr, potentialevap, Nsub, scalefactor);
    Write_OutputLine_vector(oFile[1],"results/TemperatureAve_C.txt",                  istep, tempave,       Nsub, scalefactor);
    Write_OutputLine_vector(oFile[2],"results/Surface_runoff_cms.txt",                istep, surfro,        Nsub, scalefactor);
    Write_OutputLine_vector(oFile[3],"results/Canopy_storage_mm.txt",                 istep, canstore,      Nsub, scalefactor);
    Write_OutputLine_vector(oFile[4],"results/Soil_storage_mm.txt",                   istep, soilstore,     Nsub, scalefactor);
    // -----------------------------------------------------------------------------------------------------
    Write_OutputLine_valarray(oFile[15], "results/upwelling_mm.txt", istep, dateStr, upwelling, Nsub, scalefactor);
    //Write_Line_valarray(oFile[16], "results/recharge_mm.txt",  dateStr, recharge,   Nsub, scalefactor);
    //Write_Line_valarray(extraFiles[12],  extraFileNames[12],  dateStr, precip_minus_et,   Nsub, scalefactor);
    // -----------------------------------------------------------------------------------------------------

    scalefactor = 1000.0;   // convert to mm
    double *temporary = new double[Nsub];
    for (j = 0; j < Nsub; j++) {
        temporary[j] = zbar[j][0];  // row 0; all elements of the row are the same at this point.
    }
    Write_OutputLine(oFile[5], "results/Depth_to_Water_mm.txt", istep, temporary, Nsub, scalefactor);
    delete [] temporary;

    scalefactor = 1.0;
    Write_OutputLine_vector(oFile[6], "results/Tile_drainage_cms.txt",  istep, tiled,  Nsub, scalefactor);
    Write_OutputLine_vector(oFile[7], "results/Ditch_drainage_cms.txt", istep, ditchd, Nsub, scalefactor);

    if (idebugoutput >= 1)
        lunmodFile.close();   // debugout: close model writes in case routing fails

    for (js = 0; js < maxSlp; ++js) {
        for (kk = 0; kk < nreg; ++kk) {
            delete [] dr[js][kk];
            delete [] qinst[js][kk];
        }
        delete [] dr[js];
        delete [] qinst[js];
    }
    delete [] dr;
    delete [] qinst;

    for (int i = 0; i <= m; ++i) {
        for (int j = 0; j < Nsub; ++j) {
            delete [] zbar_in[i][j];
        }
        delete [] zbar_in[i];
    }
    delete [] zbar_in;

    delete [] groundwater_to_take;

#if TRACE
    double tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
    if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving calcts(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif

    return 0;
}


// *****************************************************************
//     SUBROUTINE SNOW
// *****************************************************************
int snow(ofstream &snowOutFile, const valarray<double> &temper, const double elevTg, const double elevsb, const double rlapse, double &bRain,
         const double ddf, double &snowst, const long int dt, const int Nsub, const int m, const int js, const int it,
         const int maxSlp, const int maxInt)
{
    //SAVE
    double lastsnowst;
    double t, temp, newsnow, melt, snowbal;

#if TRACE
    static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    string save_caller = caller;
    if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> snow(" << ncalls << ")" << std::endl;
    }
    caller = "snow";
#endif

    if (it == 1) {
        //	if (js == 1) open(unit=78,file="snowout.txt")
        snowOutFile << dec << js << " " << Nsub << " " << m;
        snowOutFile << "it, precip(mm), snow(mm), rain(mm), temper(deg), snowstore(mm), melt(mm), snowbal(mm)\n";
    }

    lastsnowst = snowst;
    t = temper[it-1] - (elevsb - elevTg)*rlapse;
    temp = bRain;
    if (t < 0) {
        newsnow = bRain; //bRain in mm/interval, newsnow in mm
        snowst += newsnow; //snowst and newsnow both in mm
        bRain = 0.0;
        melt = 0.0;
    }
    else {
        newsnow = 0.0;
        melt = min(snowst, ddf*t*(double)dt/86400.0); //ddf in mm/deg/day, t in deg, dt in seconds, melt in mm
        snowst -= melt;	//melt in mm, snowst in mm
        bRain += melt; //melt in mm, bRain in mm/interval
    }
    snowbal = snowst - lastsnowst + melt - newsnow;
    snowOutFile << dec << setw(8) << it;	// unit 78
    snowOutFile << fixed << setw(12) << temp;
    snowOutFile << fixed << setw(12) << newsnow;
    snowOutFile << fixed << setw(12) << bRain;
    snowOutFile << fixed << setw(12) << t;
    snowOutFile << fixed << setw(12) << snowst;
    snowOutFile << fixed << setw(12) << melt;
    snowOutFile << fixed << setw(12) << snowbal;
#if TRACE
    double tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
    if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving snow(" << ncalls << ")" << " ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif

    return 0;
}

