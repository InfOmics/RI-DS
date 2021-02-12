/*
 * ri3.cpp
 *
 *  Created on: Aug 2, 2012
 *      Author: vbonnici
 */

/*
Copyright (c) 2015 by Rosalba Giugno

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


//#define MDEBUG



#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>

#include "c_textdb_driver.h"
#include "timer.h"


#include "AttributeComparator.h"
#include "Graph.h"
#include "MatchingMachine.h"
#include "MaMaConstrFirstDs.h"
#include "MatchListener.h"

//#define FIRST_MATCH_ONLY  //if setted, the searching process stops at the first found match
#include "Solver.h"
#include "SubGISolver.h"
#include "InducedSubGISolver.h"
#include "Domains.h"

#define PRINT_MATCHES
//#define CSV_FORMAT


using namespace rilib;

enum MATCH_TYPE {
	MT_ISO,		//isomprhism
	MT_INDSUB, //induced sub-isomorphism
	MT_MONO		//monomorphism
};

void usage(char* args0);
int match(MATCH_TYPE matchtype, GRAPH_FILE_TYPE filetype,	std::string& referencefile,	std::string& queryfile);

int main(int argc, char* argv[]){

	if(argc!=5){
			usage(argv[0]);
			return -1;
		}

	MATCH_TYPE matchtype;
	GRAPH_FILE_TYPE filetype;
	std::string reference;
	std::string query;

	std::string par = argv[1];
	if(par=="iso"){
		matchtype = MT_ISO;
	}
	else if(par=="ind"){
		matchtype = MT_INDSUB;
	}
	else
	if(par=="mono"){
		matchtype = MT_MONO;
	}
	else{
		usage(argv[0]);
		return -1;
	}

	par = argv[2];
	if(par=="gfu"){
		filetype = GFT_GFU;		//undirected type
	}
	else if(par=="gfd"){
		filetype = GFT_GFD;		//directed type
	}
	else if(par=="geu"){
		filetype = GFT_EGFU;	//undirect type with labels on edges
	}
	else if(par=="ged"){
		filetype = GFT_EGFD;	//direct type with labels on edges
	}
	//if no labels, domains are unuseful
//	else if(par=="vfu"){
//		filetype = GFT_VFU;
//	}
	else{
		usage(argv[0]);
		return -1;
	}

	reference = argv[3];
	query = argv[4];

	return match(matchtype, filetype, reference, query);
};





void usage(char* args0){
	std::cout<<"usage "<<args0<<" [iso ind mono] [gfu gfd geu ged] reference query\n";
	std::cout<<"\tmatch type:\n";
	std::cout<<"\t\tiso = isomorphism\n";
	std::cout<<"\t\tind = induced subisomorphism\n";
	std::cout<<"\t\tmono = monomorphism\n";
	std::cout<<"\tgraph input format:\n";
	std::cout<<"\t\tgfu = undirect graphs with labels on nodes\n";
	std::cout<<"\t\tgfd = direct graphs with labels on nodes\n";
	std::cout<<"\t\tgeu = undirect graphs with labels both on nodes and edges\n";
	std::cout<<"\t\tged = direct graphs with labels both on nodes and edges\n";
	std::cout<<"\treference file contains one ormore reference graphs\n";
	std::cout<<"\tquery contains the query graph (just one)\n";

};


int match(
		MATCH_TYPE 			matchtype,
		GRAPH_FILE_TYPE 	filetype,
		std::string& 		referencefile,
		std::string& 	queryfile){
	bool doBijIso = (matchtype == MT_ISO);

	TIMEHANDLE load_s, load_s_q, make_mama_s, match_s, total_s;
	double load_t=0;double load_t_q=0; double make_mama_t=0; double match_t=0; double total_t=0;
	total_s=start_time();

	int rret;

	AttributeComparator* nodeComparator;	//to compare node labels
	AttributeComparator* edgeComparator;	//to compare edges labels
	switch(filetype){
		case GFT_GFU:
		case GFT_GFD:
			//for these formats, labels are only on nodes and they are strings
			nodeComparator = new StringAttrComparator();
			//nodeComparator = new DefaultAttrComparator();
			edgeComparator = new DefaultAttrComparator();
			break;
		case GFT_EGFU:
		case GFT_EGFD:
			//labels both on nodes and edges
			nodeComparator = new StringAttrComparator();
			edgeComparator = new StringAttrComparator();
			break;
//		case GFT_VFU:
//			//no labels
//			nodeComparator = new DefaultAttrComparator();
//			edgeComparator = new DefaultAttrComparator();
//			break;
	}


	TIMEHANDLE tt_start;
	double tt_end;

#ifdef MDEBUG
	std::cout<<"reading query...\n";
#endif

	//read the query
	load_s_q=start_time();
	Graph *query = new Graph();
	rret = read_graph(queryfile.c_str(), query, filetype);
	load_t_q+=end_time(load_s_q);
	if(rret !=0){
		std::cout<<"error on reading query graph\n";
	}

#ifdef MDEBUG
	query->print();
#endif

	//delete dquery;
	load_t_q+=end_time(load_s_q);

	long 	steps = 0,				//total number of steps of the backtracking phase
			triedcouples = 0, 		//nof tried pair (query node, reference node)
			matchcount = 0, 		//nof found matches
			matchedcouples = 0;		//nof mathed pair (during partial solutions)
	long tsteps = 0, ttriedcouples = 0, tmatchedcouples = 0;

	FILE *fd = open_file(referencefile.c_str(), filetype);
	if(fd != NULL){
#ifdef PRINT_MATCHES
		//if you want to print found matches on screen
		MatchListener* matchListener=new ConsoleMatchListener();
#else
		//do not print matches
		MatchListener* matchListener=new EmptyMatchListener();
#endif

		int i=0;
		bool rreaded = true;
		do{	//for each reference graph in the file
			load_s=start_time();
			Graph * rrg = new Graph();
			//read the graph
#ifdef MDEBUG
	std::cout<<"reading reference...\n";
#endif
			int rret = read_dbgraph(referencefile.c_str(), fd, rrg, filetype);
			rreaded = (rret == 0);
			load_t+=end_time(load_s);

			if(rreaded){
				if(!doBijIso ||
					(doBijIso && (query->nof_nodes == rrg->nof_nodes))){

					//initialize domains
					sbitset *domains = new sbitset[query->nof_nodes];
					match_s=start_time();
#ifdef MDEBUG
	std::cout<<"initializing domain...\n";
#endif
					bool domok = init_domains(*rrg, *query, *nodeComparator, *edgeComparator, domains, doBijIso);
					match_t+=end_time(match_s);

					//if domain constraints are satisfied (at least one compatible target node for each query node)
					if(domok){
						std:cout<<"domain ok\n";
						//just get the domain size for each query node
						int *domains_size = new int[query->nof_nodes];
						int dsize;
						for(int ii=0; ii<query->nof_nodes; ii++){
							dsize = 0;
							for(sbitset::iterator IT = domains[ii].first_ones(); IT!=domains[ii].end(); IT.next_ones()){
								dsize++;
							}
							domains_size[ii] = dsize;

							/*std::cout<<"dsize["<<ii<<"]("<<domains_size[ii]<<")\n";
							domains[ii].print_numbers();
							std::cout<<"\n";*/
						}

#ifdef MDEBUG
	std::cout<<"building matching machine...\n";
#endif
						//build the static match machine
						make_mama_s=start_time();
						MatchingMachine* mama = new MaMaConstrFirstDs(*query, domains, domains_size);
						mama->build(*query);
						make_mama_t+=end_time(make_mama_s);

#ifdef MDEBUG
	mama->print();
#endif

						match_s=start_time();

#ifdef MDEBUG
	std::cout<<"solving...\n";
#endif
						//prepare the matching phase
						Solver* solver;
						switch(matchtype){
						case MT_ISO:  //a specialized solver for this will be better
						case MT_MONO:
							solver = new SubGISolver(*mama, *rrg, *query, *nodeComparator, *edgeComparator, *matchListener, domains, domains_size);
							break;
						case MT_INDSUB:
							solver = new InducedSubGISolver(*mama, *rrg, *query, *nodeComparator, *edgeComparator, *matchListener, domains, domains_size);
							break;
						}

						//run the matching phase
						solver->solve();

						match_t+=end_time(match_s);

						steps += solver->steps;
						triedcouples += solver->triedcouples;
						matchedcouples += solver->matchedcouples;

						delete solver;
						delete mama;
#ifdef MDEBUG
	std::cout<<"done\n";
#endif
					}
				}
//				delete rrg;
				//ReferenceGRaph destroyer is not yet developed...
			}
			i++;
		}while(rreaded);

#ifdef MDEBUG
	std::cout<<"all done\n";
#endif

		if(matchListener != NULL)
		matchcount += matchListener->matchcount;

		delete matchListener;

		fclose(fd);
	}
	else{
		std::cout<<"unable to open reference file\n";
		return -1;
	}

	total_t=end_time(total_s);

#ifdef CSV_FORMAT
	std::cout<<referencefile<<"\t"<<queryfile<<"\t";
	std:cout<<load_t_q<<"\t"<<make_mama_t<<"\t"<<load_t<<"\t"<<match_t<<"\t"<<total_t<<"\t"<<steps<<"\t"<<triedcouples<<"\t"<<matchedcouples<<"\t"<<matchcount;
#else
	std::cout<<"reference file: "<<referencefile<<"\n";
	std::cout<<"query file: "<<queryfile<<"\n";
	std::cout<<"total time: "<<total_t<<"\n";
	std::cout<<"matching time: "<<match_t<<"\n";
	std::cout<<"number of found matches: "<<matchcount<<"\n";
	std::cout<<"search space size: "<<matchedcouples<<"\n";
#endif

	delete nodeComparator;
	delete edgeComparator;

	return 0;
};





