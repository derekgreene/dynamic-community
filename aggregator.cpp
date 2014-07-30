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
// Dynamic community timeline aggregator tool
// ------------------------------------------------------------------------------------------

#include <getopt.h>
#include <math.h>
#include <string.h>
#include "settings.h"
#include "common/standard.h"
#include "common/clustering.h"
#include "dynamic.h"
#include "extras.h"
#include "aggregatorargs.h"

#define EXT_OUTPUT ".persist"
typedef map<NODE,int> FreqCluster;
typedef vector<FreqCluster> FreqClustering;

int main(int argc, char *argv[])
{
	/// Parse command line arguments
	aggregator_args_info args_info;
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
	string prefix;
	if( args_info.output_arg == NULL || strlen(args_info.output_arg) == 0 )
	{
		prefix = "dynamic";
	}
	else
	{
		prefix = string(args_info.output_arg);
	}
	double persist_threshold = args_info.persist_arg;
	if( persist_threshold < 0 || persist_threshold > 1 )
	{
		cerr << "Error: Invalid persistence threshold value: " << persist_threshold << ". Value should be between 0 and 1." << endl;
		exit(1);
	}
	bool use_union = (persist_threshold == 0 );
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
	if( supplied_steps < max_step )
	{
		cerr << "Error: incorrect number of step files specified (" << supplied_steps << " < " << max_step << ")" << endl;
		return -1;
	}
	else if( supplied_steps > max_step )
	{
		cerr << "Warning: some step files will be ignored (" << supplied_steps << " > " << max_step << ")" << endl;
	}
	if( min_persist_length < 1 )
	{
		min_persist_length = 2;
	}
	else if( min_persist_length > max_step )
	{
		cerr << "Error: invalid minimum persistent timeline length (" << min_persist_length << ")" << endl;
		return -1;
	}
	else
	{
		min_persist_length = MIN_PERSIST_LENGTH;
	}
	
	/// Filter irrelevant timelines
	Clustering persist_clustering;
	set<int> ignore_dynamic_indices;
	int filter_size = 0, filter_time = 0, filter_dead;
	for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
	{
		// add an empty cluster anyway
		Cluster persist_cluster;
		persist_clustering.push_back(persist_cluster);
		// too short?
		if( timelines[dyn_index].size() < min_persist_length )
		{
			ignore_dynamic_indices.insert(dyn_index);
			filter_size++;
		}
		// outside our time window?
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
	
	/// Process each step clustering
	if( use_union )
	{
		cout << "* Constructing persistent communities from union of step community memberships..." << endl;
		for ( int i = 0; i < max_step; ++i )
		{
			int step = i+1;
			// Read the step clustering
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
			// Update the set of persistent communities
			for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
			{
				// ignore this timeline?
				if( ignore_dynamic_indices.count(dyn_index) )
				{
					continue;
				}
				// process the step cluster in this timeline
				int step_cluster_index = timelines[dyn_index][step] - 1;
				if( step_cluster_index >= 0 )
				{
					persist_clustering[dyn_index].insert(step_clustering[step_cluster_index].begin(), step_clustering[step_cluster_index].end());
				}
			}
		}
	}
	else
	{
		int min_persist_steps = max(1, (int)round(persist_threshold*max_step) );
		cout << "* Constructing persistent communities for nodes appearing in >= " << min_persist_steps << " associated step communities ..." << endl;
		// Create frequency maps for each timeline using info from each step
		FreqClustering fclustering;
		for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
		{
			FreqCluster fcluster;
			fclustering.push_back(fcluster);
		}
		for ( int i = 0; i < max_step; ++i )
		{
			int step = i+1;
			// Read the step clustering
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
			// Update the set of persistent communities
			for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
			{
				// ignore this timeline?
				if( ignore_dynamic_indices.count(dyn_index) )
				{
					continue;
				}
				// process the step cluster in this timeline
				int step_cluster_index = timelines[dyn_index][step] - 1;
				if( step_cluster_index >= 0 )
				{
					for( Cluster::const_iterator it = step_clustering[step_cluster_index].begin(); it != step_clustering[step_cluster_index].end(); it++ )
					{
						NODE node = *it;
						fclustering[dyn_index][node] += 1;
					}
				}
			}
		}
		// Now convert frequency maps to an actual clustering
		for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
		{
			// ignore this timeline?
			if( ignore_dynamic_indices.count(dyn_index) )
			{
				continue;
			}
			for( FreqCluster::const_iterator it = fclustering[dyn_index].begin(); it != fclustering[dyn_index].end(); it++ )
			{
				if( (*it).second >= min_persist_steps )
				{
					persist_clustering[dyn_index].insert((*it).first);
				}
			}
		}
	}

	/// Remove any irrelevant persistent clusters
	int removed = remove_small_clusters(persist_clustering) - ignore_dynamic_indices.size();
	if( removed > 0 )
	{
		cout << "Removed " << removed << " group(s) of size < " << MIN_CLUSTER_SIZE  << endl;
	}
	removed = remove_duplicate_clusters(persist_clustering);
	if( removed > 0 )
	{
		cout << "Removed " << removed << " duplicate group(s)" << endl;
	}
	
	/// Write out
	string fname = prefix + EXT_OUTPUT;
	cout << "Writing " <<  persist_clustering.size() << " persistent communities to " << fname << endl;
	if( !write_clustering(fname, DEFAULT_DELIM, persist_clustering) )
	{
		cerr << "Error: Cannot write file " << fname << endl;
		return -1;
	}
	
	cout << "Done." << endl;
	return 0;
}
