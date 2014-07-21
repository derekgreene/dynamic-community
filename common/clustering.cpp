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
#include "standard.h"
#include "clustering.h"

// ----------------------------------------------------------------------------
// GENERAL CLUSTERING STATS & OPERATIONS
// ----------------------------------------------------------------------------

/**
 * Builds the set of assigned nodes in the specified clustering.
 *
 * @param clustering   the clustering to examine.
 * @param nodes        resulting set of nodes assigned to at least one cluster.
 *
 * @return number of assigned nodes.
 */
long assigned( const Clustering &clustering, set<NODE> &nodes )
{
	nodes.clear();
	Clustering::const_iterator cit;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++ )
	{
		Cluster::const_iterator it;
		for( it = (*cit).begin() ; it != (*cit).end(); it++ )
		{
			nodes.insert( *it );
		}
	}
	return (long)nodes.size();
}

/**
 * Gets the number of assigned nodes in the specified clustering.
 *
 * @param clustering   the clustering to examine.
 *
 * @return number of assigned nodes.
 */
long assigned_count( const Clustering &clustering )
{
	set<NODE> nodes;
	return assigned(clustering,nodes);
}

/**
 * Returns the number of nodes that are assigned to more than one cluster.
 *
 * @param clustering   the clustering to examine.
 *
 * @return number of nodes assigned to multiple clusters.
 */
long overlapping_count( const Clustering &clustering )
{
	long count = 0;
	set<NODE> nodes;
	Clustering::const_iterator cit;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++ )
	{
		Cluster::const_iterator it;
		for( it = (*cit).begin() ; it != (*cit).end(); it++ )
		{
			if( nodes.count( *it ) )
			{
				count++;
			}
			else
			{
				nodes.insert( *it );
			}
		}
	}
	return count;
}

/**
 * Counts the number of empty clusters in the clustering.
 *
 * @param clustering   the clustering to examine.
 *
 * @return number of empty clusters.
 */
int count_empty_clusters( const Clustering &clustering )
{
	int count = 0;
	Clustering::const_iterator cit;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++ )
	{
		if( (*cit).empty() )
		{
			count++;
		}
	}
	return count;
}

/**
 * Checks whether the specified cluster is empty.
 *
 * @param cluster   the cluster to examine.
 *
 * @return boolean value.
 */
bool is_empty( Cluster &cluster )
{
	return cluster.empty();
}

class IsSmall
{
	int _min_size;
public:
	IsSmall ( const int min_size ) : _min_size(min_size) {};
	bool operator() (const Cluster& cluster) {return cluster.size() < _min_size; }
};


/**
 * Removes all clusters from the specified clustering below a given size.
 *
 * @param clustering   the clustering to alter.
 * @param min_size     minimum size
 *
 * @return number of removed clsuters.
 */
int remove_small_clusters( Clustering &clustering, const int min_size )
{
	int previous = (int)clustering.size();
	clustering.erase( remove_if( clustering.begin(), clustering.end(), IsSmall(min_size) ), clustering.end() );
	return previous - (int)clustering.size();	
}

/**
 * Removes all duplicate clusters from the specified clustering.
 *
 * @param clustering   the clustering to alter.
 *
 * @return number of removed clsuters.
 */
int remove_duplicate_clusters( Clustering &clustering )
{
	int previous = (int)clustering.size();
	sort(clustering.begin(), clustering.end());
   clustering.erase(unique(clustering.begin(), clustering.end()), clustering.end());
	return previous - (int)clustering.size();
}

/**
 * Returns the maximum cluster size of all clusters in the specified clustering.
 *
 * @param clustering   the clustering to examine.
 *
 * @return  maximum cluster size.
 */
long max_cluster_size( const Clustering &clustering )
{
	Clustering::const_iterator cit = clustering.begin();
	long max_size = 0;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++ )
	{
		if( (*cit).size() > max_size )
		{
			max_size = (long)( (*cit).size() );
		}
	}
	return max_size;
}

// ----------------------------------------------------------------------------
// CLUSTERING INPUT/OUTPUT
// ----------------------------------------------------------------------------

/**
 * Write a clustering to the specified file, one line per cluster.
 *
 * @param fname        output file path
 * @param sep          separator character to use
 * @param clustering   the clustering to write.
 */
bool write_clustering( const string fname, const char sep, const Clustering &clustering )
{
	ofstream fout(fname.c_str()); 
	if(!fout) 
	{  
    	return false; 
   }
	Clustering::const_iterator cit = clustering.begin();
	int cluster_index = 0;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++,cluster_index++ )
	{
		if( (*cit).empty() )
		{
			cerr << "Warning: cluster " << (cluster_index+1) << "is empty. Ignoring." << endl;
			continue;
		}
		Cluster::iterator xit;
		int pos = 0;
		for( xit = (*cit).begin() ; xit != (*cit).end(); xit++ )
		{
			if( pos > 0 )
			{
				fout << sep;
			}
			fout << (*xit);
			pos++;
		}
		fout << endl;
	}
	fout.close();
	return true;
}

/**
 * Reads a clustering from the specified file, one line per cluster.
 *
 * @param fname        input file path
 * @param sep          separator character to use
 * @param clustering   the clustering to store the input.
 */
bool read_clustering( const string fname, const char sep, Clustering &clustering) 
{
	clustering.clear();
	ifstream fin(fname.c_str());
	if(!fin) 
	{  
    	return false; 
   } 
	string line;
	long num = 0;
	while(getline(fin, line, '\n') ) 
	{
		num += 1;
		Cluster cluster;
	   stringstream ss(line);
		string temp;
	   while (getline(ss, temp, sep)) 
		{  
			NODE node_index;
			stringstream is(temp);
			if( (is >> node_index).fail() )
			{
				// cerr << "Error: Invalid node index '" << is << "' appears on line " << num << ": " << temp << endl;
				//return false;
				cerr << "Warning: Skipping invalid node index '" << temp << "' on line " << num << endl;
			}
			else
			{
				cluster.insert(node_index);
			}
	   }
		if(!cluster.empty())
		{
			clustering.push_back(cluster);
		}
	}
	fin.close();
	return true;
}

/**
 * Simply print the content of a cluster to stdout.
 */
void print_cluster( Cluster &cluster )
{
	copy( cluster.begin(), cluster.end(), ostream_iterator<NODE>( cout, " " ) );
	cout << endl;
}

void print_cluster_sizes( const Clustering &clustering )
{
	cout << "Sizes: ";
	Clustering::const_iterator cit = clustering.begin();
	int cluster_index = 0;
	for( cit = clustering.begin() ; cit != clustering.end(); cit++,cluster_index++ )
	{
		cout << "C" << (cluster_index+1) << "=" << (*cit).size() << " ";
	}	
	cout << endl;
}