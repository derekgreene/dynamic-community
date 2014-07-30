/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// ------------------------------------------------------------------------------------------
// Main Dynamic Community Tracking Tool
// ------------------------------------------------------------------------------------------

#include <getopt.h>
#include <string.h>
#include "settings.h"
#include "common/standard.h"
#include "common/clustering.h"
#include "common/util.h"
#include "dynamic.h"
#include "extras.h"
#include "trackerargs.h"

int main(int argc, char *argv[])
{
	/// Parse command line arguments
	tracker_args_info args_info;
	if( cmdline_parser(argc, argv, &args_info) != 0 )
	{
		exit(1);
	}
	int max_step = args_info.inputs_num;
	if( max_step < 1 )
	{
		cerr << "Error: At least one file containing step communities should be specified" << endl;
		cmdline_parser_print_help();
		exit(1);
	}
	double matching_threshold = args_info.threshold_arg;
	if( matching_threshold < 0 || matching_threshold > 1 )
	{
		cerr << "Error: Invalid matching threshold value: " << matching_threshold << ". Value should be between 0 and 1." << endl;
		exit(1);
	}
	string prefix;
	if( args_info.output_arg == NULL || strlen(args_info.output_arg) == 0 )
	{
		prefix = "dynamic";
	}
	else
	{
		prefix = string(args_info.output_arg);
	}
	int death_age = args_info.death_arg;
	
#ifdef MAP_MATCHING
	cout << "* Applying map-based dynamic tracking (threshold=" << matching_threshold << ")" << endl;
	MapMatchingDynamicClusterer clusterer(matching_threshold,death_age);
#else
	cout << "* Applying dynamic tracking (threshold=" << matching_threshold << ")" << endl;
	MatchingDynamicClusterer clusterer(matching_threshold,death_age);
#endif

#ifdef SIM_OVERLAP
	cout << "* Using binary overlap similarity" <<  endl;
#else
	cout << "* Using Jaccard similarity" << endl;
#endif

	/// Process each time step
	clock_t start = clock();
	for ( int i = 0; i < max_step; ++i )
	{
		int step = i+1;
		string fname(args_info.inputs[i]);
		ifstream in(args_info.inputs[i]);
		if( in.is_open() == false ) 
		{
			cerr << "Error: Step communities file "<<fname<<" not found"<< endl;
			return -1;
		}
		cout << "* Loading step " << step << "/" << max_step << " from " << fname << " ..." << endl;
		Clustering step_clustering;
		if( !read_clustering( fname, DEFAULT_DELIM, step_clustering ) )
		{
			cerr << "Error: Failed to read communities from file " << fname << endl;
			return -1;
		}
		cout << "Found " << step_clustering.size() << " non-empty step communities";
#ifdef DEBUG_CLUSTERING
		cout << ", " << assigned_count(step_clustering) << " nodes assigned to a cluster, " << overlapping_count(step_clustering) << " nodes assigned to multiple clusters.";
#endif
		cout << endl;
		cout << "Matching to existing dynamic communities ..." << endl;
		clusterer.add_clustering( step_clustering );
		DynamicClustering dynamic = clusterer.find_clusters();
		cout << "Currently " << dynamic.size() << " dynamic communities, " << count_dead(dynamic, step, death_age) << " now dead." << endl;
	}
	
	/// Find final dynamic clusters and find results
	DynamicClustering dynamic = clusterer.find_clusters();
	clock_t end = clock();
	cout << "* Overall: Tracked " << dynamic.size() << " dynamic communities, " << count_dead(dynamic, max_step+death_age, death_age) << " now dead." << endl;
	cout << "Total time: " << diff_clock(end,start)/1000 << " sec"<<endl;
			
	/// Write the results
#ifdef ENABLE_WRITING			
	string fname = prefix + ".timeline";
	cout << "Writing timeline to " << fname << endl;
	if( !write_timelines(fname, dynamic) )
	{
		cerr << "Error: Cannot write file " << fname << endl;
		return -1;
	}
#endif
			
	cout << "Done." << endl;
	return 0;
}
