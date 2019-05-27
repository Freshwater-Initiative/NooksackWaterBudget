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

using namespace data_array;
using namespace constant_definitions; //define names for numeric codes (types of nodes, links, users, etc)
using namespace other_structures;
using namespace std;

using namespace input_structures;
using namespace TimeVaryingOutput;

int writeWithdrawalByUserType(const string dirname, const int NumUser, const int NumTimesteps)
{
	int i, j, nc, ncode, ncol;

	string filenm, LocationTypeString;
	ostringstream location,codeStr;

#if TRACE
	static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
	string save_caller = caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> writeWithdrawalByUserType(" << ncalls << ")" << std::endl;
    }
	caller = "writeWithdrawalByUserType";
#endif

	int code[] = {  SoilMoistureIrrigationUseCode,      // 1
                    FixedDemandIrrigationUseCode,       // 2
                    DownstreamReservoirReleaseUseCode,  // 3
                    PWSUseCode,                         // 4
                    NonPWSMandIUseCode,                 // 5
                    DairyUseCode};                      // 6

    for (ncode = 0; ncode < 6; ++ncode) {
        codeStr << code[ncode];

        LocationTypeString = "       User";
        filenm = dirname + "/" + "UserType_" + codeStr.str() + "_Withdrawal_cms.txt";
        codeStr.clear();
        codeStr.str("");
        ofstream outFile(filenm.c_str());
        if (!outFile.is_open()) {
			cerr << "Failed to open '" << filenm << "': exiting.\n";
			exit(EXIT_FAILURE);
        }
        //cout << "Writing to '" << filenm << "'\n";
        outFile << setw(18) << "TimeStep";
        ncol = 0;
        for (i = 1; i <= NumUser; i++) {
            if (User[i-1].UsersType == code[ncode]) {
                location << dec << setw(3) << i;
                outFile << setw(18) << LocationTypeString + location.str();
                location.clear();
                location.str("");
                ncol++;
            }
        }
        outFile << '\n';

        vector<vector<double> > data_array(NumTimesteps,vector<double>(ncol));
        nc = 0;
        for (i = 0; i < NumUser; i++) {
            if (User[i].UsersType == code[ncode]) {
                for (j = 0; j < NumTimesteps; j++) {
                    data_array[j][nc] = User[i].Withdrawal[j]/86400.0;
                }
                nc++;
            }
        }

        // Now reverse the order of the loops from i,j to j,i.
        for (j = 0; j < NumTimesteps; j++) {
            outFile << dec << setw(18) << j+1;
            for (i = 0; i < ncol; i++) {
                outFile << fixed << setw(18) << setprecision(7) << data_array[j][i];
            }
            outFile << '\n';
        }

        outFile.close();
	}

#if TRACE
	double tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving writeWithdrawalByUserType(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif
	return 0;
}
