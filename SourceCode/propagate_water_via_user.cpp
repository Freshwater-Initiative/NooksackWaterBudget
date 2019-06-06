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

using namespace constant_definitions;
using namespace input_structures;
using namespace other_structures;
using namespace std;

int PropagateWaterViaUser(const int i, const int j, const double Qtry, const int NumNode,
	const int NumLink, const int NumUser, const int NumReservoir, const int NumSource,
	int *DrainageOrder, const int NumDrainage, const int NumReturnFlow, int &iFeasible, double &Capacity)
{
	vector<double> RFAmt;
	double RF = 0.0;
	vector<int> RFUnits, RFType, RFLocn;
	int ii, jj, j_sink, mm, n, j_r, j_drainage, j_return, NumReturnFlows, nfound, *ifound;
	bool found;
	//int k, SrcLocnID, RFNodeID;

	valarray<double> DrainageOutFlow(NumDrainage);	// This is a local array.
	vector<double> ReservoirNetStorage(NumReservoir);

#if TRACE
	static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    string save_caller = caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> PropagateWaterViaUser(" << ncalls << ")" << std::endl;
    }
        caller = "PropagateWaterViaUser";
#endif

	//k is node of this user
	//k = User[i-1].NodeNumber;
	//jj is link from source to user
	jj = User[i-1].LinkSourceToUser[j-1];
	Link[jj-1].Flow += Qtry; //supply flow from source to user

	//j_sink= link from user node to sink node
	j_sink = User[i-1].LinkSourceToSink;
	Link[j_sink-1].Flow += Qtry; //consumption from user to sink

	if (User[i-1].ReturnFlowID > 0) {
		// find1()
		found = false;
		for (n = 0; n < NumReturnFlow; n++) {
			if (ReturnFlow[n].ReturnFlowID == User[i-1].ReturnFlowID) {
				j_r = n+1;
				if (!found) {
					found = true;
				} else {
					cerr << "PropagateWaterViaUser(): Duplicate link found" << endl;
					exit(EXIT_FAILURE);
				}
			}
		}
		if (!found) {
			cerr << "PropagateWaterViaUser() find1() failure\n";
			exit(EXIT_FAILURE);
		} else {
			// reset
			found = false;
		}

		NumReturnFlows = ReturnFlow[j_r-1].NumReturnFlows;
		RFAmt.resize(NumReturnFlows);
		RFUnits.resize(NumReturnFlows);
		RFType.resize(NumReturnFlows);
		RFLocn.resize(NumReturnFlows);
		for (mm = 1; mm <= NumReturnFlows; mm++) {
			RFAmt[mm-1]   = ReturnFlow[j_r-1].ReturnFlowsAmt[mm-1];
			RFUnits[mm-1] = ReturnFlow[j_r-1].ReturnFlowsUnits;
			RFType[mm-1]  = ReturnFlow[j_r-1].ReturnFlowsType[mm-1];
			if (RFType[mm-1] == 0)
				RFType[mm-1] = StreamNodeCode;
			RFLocn[mm-1] = ReturnFlow[j_r-1].ReturnFlowsLocn[mm-1];
			if (RFLocn[mm-1] == 0)
				RFLocn[mm-1] = User[i-1].POU_ID;
			if (RFType[mm-1] == 1 && RFLocn[mm-1] == -1) {
				// find1()
				nfound = 0; //none found
				ifound = new int[NumDrainage];
				ifound[nfound] = 0;
				for (n = 0; n < NumDrainage; n++) {
					if (Drainage[n].DrainageID == User[i-1].POU_ID) {
						nfound++;
						ifound[nfound-1] = n+1;
					}
				}
				j_drainage = ifound[0];	//problem if nfound<>1
				delete [] ifound;
				RFLocn[mm-1] = Drainage[j_drainage-1].DSDrainage;
			}
		}
	} else if (User[i-1].ReturnFlowID < 0) {
		NumReturnFlows = 0;
	} else {
		NumReturnFlows = 1;
		RFAmt.resize(NumReturnFlows);
		RFUnits.resize(NumReturnFlows);
		RFType.resize(NumReturnFlows);
		RFLocn.resize(NumReturnFlows);
		RFAmt[0]   = 1;
		RFUnits[0] = 1;
		RFType[0]  = StreamNodeCode;
		RFLocn[0]  = User[i-1].POU_ID;
	}

	for (mm = 0; mm < NumReturnFlows; mm++) { //length(User(i)%ReturnFlowFrac) return flow(s) from user to return flow nodes
		if (RFUnits[mm] == FractionUnits) {
			RF = Qtry*RFAmt[mm];
		} else if (RFUnits[mm] == VolumeUnits) {
			RF = RFAmt[mm];
		} else if (RFUnits[mm] == FracMinDemandUnits) {
			std::cout << "FracMinDemand not yet implemented\n";
			RF = Qtry*RFAmt[mm];
		}

		//j_return= link from user node to returnflow node
		j_return = User[i-1].LinkUserToReturnflow[mm];
		//    do ii=1,nfound
		Link[j_return-1].Flow += RF;
		//    end do
		Link[j_sink-1].Flow -= RF;
#ifdef DEBUG
if (timeinfo->tm_mon == 8 && timeinfo->tm_mday == 30) {
        debugFile2 << ", Propagate via user, return flow: Link[" << dec << setw(3) << j_return << "].Flow=";  //debug
        debugFile2 << fixed << setw(13) << setprecision(6) << Link[j_return-1].Flow << endl;
}
#endif
	}

	BalanceFlowsAtReservoirs(NumNode, NumLink, NumUser, NumReservoir, ReservoirNetStorage);
	BalanceFlowsAtStreamNodes(DrainageOrder, NumDrainage, DrainageOutFlow);
	iFeasible = 1;
	double minDrainage = 1.0e10, minReservoir = 1.0e10;
	for (ii = 0; ii < NumDrainage; ii++) {
		if (DrainageOutFlow[ii] < 0.0) {
			iFeasible = 0;
		}
		if (minDrainage > DrainageOutFlow[ii]) {
			minDrainage = DrainageOutFlow[ii];
		}
	}
	for (ii = 0; ii < NumReservoir; ii++) {

		if (ReservoirNetStorage[ii] < 0.0) {
			iFeasible = 0;
		}
		if (minReservoir > ReservoirNetStorage[ii]) {
			minReservoir = ReservoirNetStorage[ii];
		}
	}
	Capacity = min(minDrainage, minReservoir);
#if TRACE
    double tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
    if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving PropagateWaterViaUser(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif

	return 0;
}
