/*
 * Solver.h
 *
 *  Created on: Aug 4, 2012
 *      Author: vbonnici
 */
/*
Copyright (c) 2014 by Rosalba Giugno

This library contains portions of other open source products covered by separate
licenses. Please see the corresponding source files for specific terms.

RI is provided under the terms of The MIT License (MIT):

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef SOLVER_H_
#define SOLVER_H_

#include "MatchingMachine.h"
#include "Graph.h"
#include "sbitset.h"

namespace rilib{

class Solver{
public:
	MatchingMachine& mama;
	Graph& rgraph;
	Graph& qgraph;
	AttributeComparator& nodeComparator;
	AttributeComparator& edgeComparator;
	MatchListener& matchListener;
	sbitset *domains;
	int *domains_size;



	long steps;
	long triedcouples;
	long matchedcouples;

public:

	Solver(
			MatchingMachine& _mama,
			Graph& _rgraph,
			Graph& _qgraph,
			AttributeComparator& _nodeComparator,
			AttributeComparator& _edgeComparator,
			MatchListener& _matchListener,
			sbitset *_domains,
			int *_domains_size
			)
			: mama(_mama),
			  rgraph(_rgraph),
			  qgraph(_qgraph),
			  nodeComparator(_nodeComparator),
			  edgeComparator(_edgeComparator),
			  matchListener(_matchListener),
			  domains(_domains),
			  domains_size(_domains_size){
		steps = 0;
		triedcouples = 0;
		matchedcouples = 0;
	}

	virtual ~Solver(){}


	void solve(){

		int ii;

		int nof_sn 						= mama.nof_sn;
		void** nodes_attrs 				= mama.nodes_attrs;				//indexed by state_id
		int* edges_sizes 				= mama.edges_sizes;				//indexed by state_id
		MaMaEdge** edges 				= mama.edges;					//indexed by state_id
		int* map_node_to_state 			= mama.map_node_to_state;			//indexed by node_id
		int* map_state_to_node 			= mama.map_state_to_node;			//indexed by state_id
		int* parent_state 				= mama.parent_state;			//indexed by state_id
		MAMA_PARENTTYPE* parent_type 	= mama.parent_type;				//indexed by state id


		int** candidates = new int*[nof_sn];							//indexed by state_id
		int* candidatesIT = new int[nof_sn];							//indexed by state_id
		int* candidatesSize = new int[nof_sn];							//indexed by state_id
		int* solution = new int[nof_sn];								//indexed by state_id
		for(ii=0; ii<nof_sn; ii++)
			solution[ii] = -1;

		bool* matched = (bool*) calloc(rgraph.nof_nodes, sizeof(bool));		//indexed by node_id

		for(int i=0; i<nof_sn; i++){
			if(parent_type[i] == PARENTTYPE_NULL){
				int n = map_state_to_node[i];
				candidates[i] = new int[domains_size[n]];

				int k = 0;
				for(sbitset::iterator IT = domains[n].first_ones(); IT!=domains[n].end(); IT.next_ones()){
					candidates[i][k] = IT.first;
					k++;
				}

				candidatesSize[i] = domains_size[n];
				candidatesIT[i] = -1;
			}
		}


		int psi = -1;
		int si = 0;
		int ci = -1;
		int sip1;
		while(si != -1){

			//std::cout<<"si("<<si<<") ["<<map_state_to_node[si]<<"] r["<<map_node_to_state[map_state_to_node[si]]<<"]\n";
			//steps++;

			if(psi >= si){
				matched[solution[si]] = false;
			}

			ci = -1;
			candidatesIT[si]++;
			while(candidatesIT[si] < candidatesSize[si]){
				//triedcouples++;

				ci = candidates[si][candidatesIT[si]];
				solution[si] = ci;

#ifdef MDEBUG
	if(!matched[ci]){
		std::cout<<"trying ("<<map_state_to_node[si]<<","<<ci<<")\n";
		if(!domains[map_state_to_node[si]].get(ci))
			std::cout<<"\tfails on domains\n";
		if(!edgesCheck(si, ci, solution, matched))
			std::cout<<"\tfails on edges\n";
	}
#endif


				if(		(!matched[ci])
				    //  && nodeCheck(si,ci, map_state_to_node)
						&& domains[map_state_to_node[si]].get(ci)
				      && edgesCheck(si, ci, solution, matched)
				            ){
					break;
				}
				else{
					ci = -1;
				}
				candidatesIT[si]++;
			}

			if(ci == -1){
				psi = si;
				si--;
			}
			else{
				matchedcouples++;

				if(si == nof_sn -1){
					matchListener.match(nof_sn, map_state_to_node, solution);
					psi = si;
#ifdef FIRST_MATCH_ONLY
					si = -1;
#endif
//					return IF U WANT JUST AN INSTANCE;
				}
				else{
					matched[solution[si]] = true;
					sip1 = si+1;
					if(parent_type[sip1] == PARENTTYPE_NULL){
					}
					else{
						if(parent_type[sip1] == PARENTTYPE_IN){
							candidates[sip1] = rgraph.in_adj_list[solution[parent_state[sip1]]];
							candidatesSize[sip1] = rgraph.in_adj_sizes[solution[parent_state[sip1]]];
						}
						else{//(parent_type[sip1] == MAMA_PARENTTYPE::PARENTTYPE_OUT)
							candidates[sip1] = rgraph.out_adj_list[solution[parent_state[sip1]]];
							candidatesSize[sip1] = rgraph.out_adj_sizes[solution[parent_state[sip1]]];
						}
					}
					candidatesIT[si +1] = -1;

					psi = si;
					si++;
				}
			}

		}
	}


//	virtual bool nodeCheck(int si, int ci, int* map_state_to_node)=0;
	virtual bool edgesCheck(int si, int ci, int* solution, bool* matched)=0;


};

}


#endif /* SOLVER_H_ */
