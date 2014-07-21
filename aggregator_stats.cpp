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
// Dynamic community timeline aggregator stats tool
// ------------------------------------------------------------------------------------------

#include <getopt.h>
#include <string.h>
#include "settings.h"
#include "common/standard.h"
#include "common/clustering.h"
#include "settings.h"
#include "dynamic.h"
#include "extras.h"
#include "aggregator_statsargs.h"

int main(int argc, char *argv[])
{
	/// Parse command line arguments
	aggregator_stats_args_info args_info;
	if( cmdline_parser(argc, argv, &args_info) != 0 )
	{
		exit(1);
	}
	int supplied_steps = args_info.inputs_num;
	if( supplied_steps < 1 )
	{
		cerr << "Error: At least one file containing step communities should be specified" << endl;
		cmdline_parser_print_help();
		exit(1);
	}
	if( args_info.input_arg == NULL || strlen(args_info.input_arg) == 0 )
	{
		cerr << "Error: No input timeline file path specified." << endl;
		exit(1);
	}
	int user_max_step = args_info.max_arg;
	int min_persist_length = args_info.length_arg;
	string timeline_fname = string(args_info.input_arg);
	
	/// Read timeline
	vector<Timeline> timelines;
	int limit_max_step;
	cout << "* Loading timelines from " << timeline_fname << endl;
	if( !read_timelines( timeline_fname, timelines, limit_max_step ) )
	{
		cerr << "Error: Failed to read timelines from file " << timeline_fname << endl;
		return -1;
	}
	cout << "Read " << timelines.size() << " dynamic community timelines" << endl;
	int max_step;
	if( user_max_step > 0 && user_max_step < limit_max_step )
	{
		cout << "Timeline will be truncated at step " << user_max_step << endl;
		max_step = min(limit_max_step,user_max_step);
	}
	else
	{
		max_step = limit_max_step;
	}
	if( min_persist_length < 1 || min_persist_length > max_step )
	{
		cerr << "Error: invalid minimum persistent timeline length (" << min_persist_length << ")" << endl;
		return -1;
	}
	else
	{
		min_persist_length = MIN_PERSIST_LENGTH;
	}
	
	/// Filter irrelevant communities
	Clustering union_clustering;
	vector<int> lengths;
	set<int> ignore_dynamic_indices;
	int filter_size = 0, filter_time = 0, filter_dead;
	for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
	{
		int seen = (int)(timelines[dyn_index].size());
		lengths.push_back( seen );
		Cluster union_cluster;
		union_clustering.push_back(union_cluster);
		// too short?
		if( timelines[dyn_index].size() < min_persist_length )
		{
			ignore_dynamic_indices.insert(dyn_index);
			filter_size++;
		}
		else if( timelines[dyn_index].first_observed() > max_step )
		{
			ignore_dynamic_indices.insert(dyn_index);
			filter_time++;
		}
	}
	if( filter_size > 0 )
	{
		cout << "Ignoring " << filter_size << " dynamic communities of duration < " << min_persist_length << endl;
	}
	if( filter_time > 0 )
	{
		cout << "Ignoring " << filter_time << " inactive communities" << endl;
	}
	
	/// Process each set of step communities
	vector<Timeline>::iterator timeit;
	if( supplied_steps < max_step )
	{
		cerr << "Error: incorrect number of step files specified (" << supplied_steps << " < " << max_step << ")" << endl;
		return -1;
	}
	else if( supplied_steps > max_step )
	{
		cerr << "Warning: some step files will be ignored (" << supplied_steps << " > " << max_step << ")" << endl;
	}
	for ( int i = 0; i < max_step; ++i )
	{
		int step = i+1;
		string fname(args_info.inputs[i]);
		ifstream in(args_info.inputs[i]);
		if(in.is_open() == false) 
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
		cout << "Found " << step_clustering.size() << " non-empty step communities" << endl;
		// create the set
		for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
		{
			// ignore?
			if( ignore_dynamic_indices.count(dyn_index) )
			{
				continue;
			}
			int step_cluster_index = timelines[dyn_index][step] - 1;
			if( step_cluster_index >= 0 )
			{
				union_clustering[dyn_index].insert(step_clustering[step_cluster_index].begin(), step_clustering[step_cluster_index].end());
			}
		}
	}
	
	printf("Found %d total dynamic communities. Ignoring %d dynamic communities.\n", (int)timelines.size(), (int)ignore_dynamic_indices.size() );
	for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
	{
		// ignore?
		if( ignore_dynamic_indices.count(dyn_index) )
		{
			continue;
		}
		printf("D%d: UnionSize=%ld Observations=%d\n", dyn_index+1, (long)union_clustering[dyn_index].size(), lengths[dyn_index] );
	}

	/*
	/// Remove any irrelevant union clusters
	int removed = remove_small_clusters(union_clustering) - ignore_dynamic_indices.size();
	if( removed > 0 )
	{
		cout << "Removed " << removed << " group(s) of size < " << MIN_CLUSTER_SIZE  << endl;
	}
	removed = remove_duplicate_clusters(union_clustering);
	if( removed > 0 )
	{
		cout << "Removed " << removed << " duplicate group(s)" << endl;
	}
	*/
	
	cout << "Done." << endl;
	return 0;
}
