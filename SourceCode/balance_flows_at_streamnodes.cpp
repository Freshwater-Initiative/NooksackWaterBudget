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

int BalanceFlowsAtStreamNodes(int *DrainageOrder, const int NumDrainage, valarray<double> &DrainageOutFlow)
{
	array<int,1000> ifound_in, ifound_taken;
	int n, i, j, ii, n_in, n_taken, j_unallocated;
	double flow_in, flow_taken;

#if TRACE
	static int ncalls = 0;
    double tm0 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
	string save_caller = caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " -> BalanceFlowsAtStreamNodes(" << ncalls << ")" << std::endl;
    }
	caller = "BalanceFlowsAtStreamNodes";
#endif
	//Calculate Flow in all links
	//  Process each stream node in drainage calculation order (from upstream to downstream) assigning the flow to the outgoing
	//  unallocated link the sum of all inflows minus the sum of all withdrawals.
	for (ii = 1; ii <= NumDrainage; ii++) {
		i = DrainageOrder[ii-1];
		n_in = Drainage[i-1].n_in;

		for (n = 0; n < n_in; n++) {
			ifound_in[n] = Drainage[i-1].ifound_in[n];
		}
		n_taken = Drainage[i-1].n_taken;

		for (n = 0; n < n_taken; n++) {
			ifound_taken[n] = Drainage[i-1].ifound_taken[n];
		}
		j_unallocated = Drainage[i-1].j_unallocated;

		flow_in = 0.0;
		for (j = 0; j < n_in; j++) {
			flow_in += Link[ifound_in[j]-1].Flow;
		}
		flow_taken = 0.0;
		for (j = 0; j < n_taken; j++) {
			flow_taken += Link[ifound_taken[j]-1].Flow;
		}

		Link[j_unallocated-1].Flow = flow_in - flow_taken;
		DrainageOutFlow[i-1] = Link[j_unallocated-1].Flow;
	}

#if TRACE
	double tm1 = static_cast<double>(clock())/static_cast<double>(CLOCKS_PER_SEC);
    caller = save_caller;
	if (ncalls < MAX_TRACE) {
        traceFile << setw(30) << caller << " <- Leaving BalanceFlowsAtStreamNodes(" << ncalls << ") ";
        traceFile << tm1 - tm0 << " seconds\n\n";
    }
    ncalls++;
#endif

	return 0;
}
