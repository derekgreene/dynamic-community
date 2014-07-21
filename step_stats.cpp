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
		cerr << "Usage: " << argv[0] << " step1_communities step2_communities..." << endl;
		return -1;
	}
	int max_step = argc - 1;

	// Get all nodes
	set<NODE> assigned;
	for( int step = 1; step <= max_step; step++ )
	{
		string fname(argv[step]);
		ifstream in(argv[step]);
		Clustering clustering;
		if( !read_clustering( fname, DEFAULT_DELIM, clustering ) )
		{
			cerr << "Error: Failed to read communities from file " << fname << endl;
			return -1;
		}
		cout << "Step " << step << ": " << clustering.size() << " non-empty step communities" << endl;
		Clustering::const_iterator cit;
		for( cit = clustering.begin() ; cit != clustering.end(); cit++ )
		{
			Cluster::const_iterator it;
			for( it = (*cit).begin() ; it != (*cit).end(); it++ )
			{
				assigned.insert(*it);
			}
		}
	}
	long n = (long)(assigned.size());
	cout << "Total nodes assigned: " << n << endl;
	for( int step = 1; step <= max_step; step++ )
	{
		string fname(argv[step]);
		ifstream in(argv[step]);
		Clustering clustering;
		if( !read_clustering( fname, DEFAULT_DELIM, clustering ) )
		{
			cerr << "Error: Failed to read communities from file " << fname << endl;
			return -1;
		}
		long step_assigned = assigned_count(clustering);
		double per_assigned = 100 * ((double)step_assigned)/n;
		printf("Step %d: %ld/%ld nodes assigned (%.1f%%)\n", step, step_assigned, n, per_assigned );
	}
	
	cout << "Done." << endl;
	return 0;
}
