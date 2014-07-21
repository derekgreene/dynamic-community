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

#include "settings.h"
#include "common/standard.h"
#include "common/clustering.h"
#include "dynamic.h"
#include "extras.h"

int main(int argc, char *argv[])
{
	if( argc < 2 )
	{
		cerr << "Error: Invalid number of arguments." << endl;
		cerr << "Usage: " << argv[0] << " [timeline_file] step1_communities step2_communities..." << endl;
		return -1;
	}
		
	// Read timeline
	string timeline_fname(argv[1]);
	vector<Timeline> timelines;
	int max_step;
	cout << "* Loading timelines from " << timeline_fname << endl;
	if( !read_timelines( timeline_fname, timelines, max_step ) )
	{
		cerr << "Error: Failed to read timelines from file " << timeline_fname << endl;
		return -1;
	}
	cout << "Read " << timelines.size() << " dynamic community timelines" << endl;
	
	/// Create storage
	Clustering union_clustering;
	vector<int> lengths;
	for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
	{
		int seen = (int)(timelines[dyn_index].size());
		lengths.push_back( seen );
		Cluster union_cluster;
		union_clustering.push_back(union_cluster);
	}
	
	/// Process each set of step communities
	vector<Timeline>::iterator timeit;
	int supplied_steps = argc - 2;
	if( supplied_steps < max_step )
	{
		cerr << "Error: incorrect number of step files specified (" << supplied_steps << " < " << max_step << ")" << endl;
		return -1;
	}
	for( int step = 1; step <= max_step; step++ )
	{
		string fname(argv[step+1]);
		ifstream in(argv[step+1]);
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
			int step_cluster_index = timelines[dyn_index][step] - 1;
			if( step_cluster_index >= 0 )
			{
				union_clustering[dyn_index].insert(step_clustering[step_cluster_index].begin(), step_clustering[step_cluster_index].end());
			}
		}
	}
	
	set<NODE> all_nodes;
	for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
	{
		Cluster::const_iterator it;
		for( it = union_clustering[dyn_index].begin() ; it != union_clustering[dyn_index].end(); it++ )
		{
			all_nodes.insert( *it );
		}
	}
	long total_nodes = (long)all_nodes.size();
	printf("%ld nodes assigned in total.\n", total_nodes);
	
	set<NODE> assigned;
	set<NODE> present_comms;
	map<NODE,int> nmap;
	for( int i = max_step; i > 0; i--)
	{
		for( int dyn_index = 0; dyn_index < timelines.size(); dyn_index++ )
		{
			if( lengths[dyn_index] == i )
			{
				Cluster::const_iterator it;
				for( it = union_clustering[dyn_index].begin() ; it != union_clustering[dyn_index].end(); it++ )
				{
					NODE node = *it;
					assigned.insert( node );
					if( nmap.find( node ) == nmap.end() )
					{
						nmap[node] = 1;
					}
					else
					{
						nmap[node] += 1;
					}
				}
				present_comms.insert( dyn_index );
			}
		}
		int comm_count = (int)present_comms.size();
		double frac_comm = 100*(((double)comm_count)/(int)(timelines.size()));
		long assigned_count = (long)(assigned.size());
		double frac_nodes = 100*(((double)assigned_count)/total_nodes);		
		printf( "  Present in at least %d consecutive step(s): %d communities (%.1f%%), %ld nodes (%.1f%%)\n", i, comm_count, frac_comm, assigned_count, frac_nodes );
		
		map<NODE,int>::const_iterator it;
		double total = 0.0;
		int max_per_comm = 0;
		for ( it = nmap.begin() ; it != nmap.end(); it++ )
		{
			int over = (*it).second;
	 		total += over;
			if( over > max_per_comm )
			{
				max_per_comm = over; 
			}
		}	
		total /= (int)nmap.size();
		printf("  Communities per Node: mean=%.2f max=%d\n",total,max_per_comm);
	}
	
	
	cout << "Done." << endl;
	return 0;
}
