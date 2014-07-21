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

#include "common/standard.h"
#include "settings.h"
#include "dynamic.h"
#include "extras.h"

// ------------------------------------------------------------------------------------------
// Class: Timeline
// ------------------------------------------------------------------------------------------

Timeline::Timeline( vector<int> &m_history_step, vector<int> &history_cluster )
	: m_history_step(m_history_step), m_history_cluster(history_cluster)
{
}

int Timeline::last_observed() const
{
	return m_history_step.back();
}

int Timeline::first_observed() const
{
	if( m_history_step.empty() )
	{
		return -1;
	}
	return m_history_step[0];
}

/**
 * Returns the maximum consecutive number of time steps for which this community is observed.
 */
int Timeline::consecutive_length() const
{
	if( m_history_step.empty() )
	{
		return 0;
	}
	int max_consec = 1;
	int cur_consec = 0;
	int last_step = 0;
	for( int i = 0; i < m_history_step.size(); i++ )
	{
		int step = m_history_step[i];
		// First or consecutive?
		if( i == 0 || step == last_step + 1 )
		{
			cur_consec++;
		}
		else
		{
			max_consec = max(cur_consec,max_consec);
			cur_consec = 0;
		}
		last_step = step;
	}
	max_consec = max(cur_consec,max_consec);
	return max_consec;
}

bool Timeline::is_dead( const int step, const int death_age ) const
{
	return (step - m_history_step.back() - 1) >= death_age;
}

bool Timeline::is_observed( const int step ) const
{
	vector<int>::const_iterator result = find( m_history_step.begin(), m_history_step.end(), step );
	return (result != m_history_step.end());
}

int Timeline::operator[] (const int step) const
{
	for( int i = 0; i < m_history_step.size(); i++ )
	{
		if( m_history_step[i] == step )
		{
			return m_history_cluster[i];
		}
	}
	return -1;
}

int Timeline::size() const
{
	return (int)m_history_step.size();	
}

// ------------------------------------------------------------------------------------------
// Class: DynamicCluster
// ------------------------------------------------------------------------------------------

DynamicCluster::DynamicCluster( DynamicCluster &sibling, const int step, const int step_cluster_index, Cluster &initial_front )
	: Timeline()
{
	// copy sibling up to previous time step
	if( sibling.m_history_step.size() > 0 )
	{
		m_history_step.resize(sibling.m_history_step.size()-1);
		copy(sibling.m_history_step.begin(), sibling.m_history_step.end()-1, m_history_step.begin());
		m_history_cluster.resize(sibling.m_history_cluster.size()-1);
		copy(sibling.m_history_cluster.begin(), sibling.m_history_cluster.end()-1, m_history_cluster.begin());
	}
	// and add unique
	update( step, step_cluster_index, initial_front );
}
	
inline bool DynamicCluster::update( const int step, const int step_cluster_index, Cluster &initial_front ) 
{
	if( size() > 0 && step <= last_observed() )
	{
		cerr << "Warning: History out of sync. New step <= last step. (" << step << " < " << last_observed() << endl;
		return false;
	}
	m_front = initial_front;
	m_history_step.push_back( step );
	m_history_cluster.push_back( step_cluster_index ); 
	return true;
}

Cluster &DynamicCluster::front()
{
	return m_front;
}

// ------------------------------------------------------------------------------------------
// Class: MatchingDynamicClusterer
// ------------------------------------------------------------------------------------------

MatchingDynamicClusterer::MatchingDynamicClusterer( const double matching_threshold, const int death_age ) 
	: m_threshold(matching_threshold), m_death_age(death_age), m_step(0)
{
}

DynamicClustering &MatchingDynamicClusterer::find_clusters()
{
	return m_dynamic;
}

bool MatchingDynamicClusterer::add_clustering( Clustering &step_clustering )
{
	m_step += 1;
	/// First?
	if( m_step == 1 )
	{
		return bootstrap(step_clustering);
	}
	
	/// Otherwise, try to match all
	Clustering::iterator cit;
	Clustering::iterator cend = step_clustering.end();
	int step_cluster_index = 0;
	vector<DynamicCluster> fresh;
	PairVector matched_pairs;
	for( cit = step_clustering.begin() ; cit != cend; cit++, step_cluster_index++ )
	{
		vector<int> matches;
		find_matches( *cit, matches );
		// new community?
		if( matches.empty() )
		{
			DynamicCluster dc;
			dc.update( m_step, step_cluster_index, *cit );
			fresh.push_back(dc);
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Birth: Community M" << (m_dynamic.size()+fresh.size()) << " from C" << step_cluster_index+1 << endl;
#endif			
		}
		else
		{
			vector<int>::const_iterator iit;
			for( iit = matches.begin() ; iit != matches.end(); iit++ )
			{
				pair<int,int> p(step_cluster_index,(*iit));
				matched_pairs.push_back(p);
			}
		}
	}
	
	// Actually update existing dynamic communities now
	set<int> matched_dynamic;
	PairVector::const_iterator pit;
	for( pit = matched_pairs.begin(); pit != matched_pairs.end(); pit++ )
	{
		int step_cluster_index = (*pit).first;
		int dyn_cluster_index = (*pit).second;
		// already processed this dynamic cluster?
		if( matched_dynamic.count( dyn_cluster_index ) ) 
		{
			DynamicCluster dc( m_dynamic[dyn_cluster_index], m_step, step_cluster_index, step_clustering[step_cluster_index] );
			fresh.push_back(dc);
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Split: Matched C" << (step_cluster_index+1) << " to M" << (dyn_cluster_index+1) << ". Splitting to M" << (m_dynamic.size()+fresh.size()) <<  endl;
#endif
		}
		else
		{
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Continuation: Matched C" << (step_cluster_index+1) << " to M" << (dyn_cluster_index+1) << endl;
#endif
			m_dynamic[dyn_cluster_index].update( m_step, step_cluster_index, step_clustering[step_cluster_index] );
			matched_dynamic.insert(dyn_cluster_index);
		}
	}
	// And finally add any new dynamic communities
	DynamicClustering::const_iterator dit;
	for( dit = fresh.begin() ; dit != fresh.end(); dit++ )
	{
		m_dynamic.push_back(*dit);
	}
	
	return true;
}

bool MatchingDynamicClusterer::bootstrap( Clustering &step_clustering )
{
	m_dynamic.clear();
	Clustering::iterator cit;
	Clustering::iterator cend = step_clustering.end();
	int step_cluster_index = 0;
	for( cit = step_clustering.begin() ; cit != cend; cit++, step_cluster_index++ )
	{
		// if( (*cit).size() < MIN_CLUSTER_SIZE || (*cit).size() > MAX_CLUSTER_SIZE )
		if( (*cit).size() < MIN_CLUSTER_SIZE )
		{
			continue;
		}
		DynamicCluster dc;
		dc.update( m_step, step_cluster_index, *cit );
		m_dynamic.push_back(dc);
#ifdef DEBUG_MATCHING
		cout << "T" << m_step << ": Birth: Community M" << m_dynamic.size() << endl;
#endif			
	}
	return true;
}

inline void MatchingDynamicClusterer::find_matches( const Cluster &step_cluster, vector<int> &matches )
{
	int size_step = (int)step_cluster.size();
	if( size_step < MIN_CLUSTER_SIZE )
	{
		return;
	}
	DynamicClustering::iterator dit;
	DynamicClustering::iterator dend = m_dynamic.end();
	int dyn_index = 0;
	for( dit = m_dynamic.begin() ; dit != dend; dit++, dyn_index++ )
	{
		// Dead?
		if( m_death_age > 0 && m_dynamic[dyn_index].is_dead( m_step, m_death_age ) )
		{
			continue;
		}
		Cluster& front = (*dit).front();
		int size_front = front.size();
		set<NODE> tmp;
     	set_intersection(step_cluster.begin(), step_cluster.end(), front.begin(), front.end(), insert_iterator< set < NODE > > (tmp,tmp.begin()) );
		int inter = tmp.size();
		if( inter == 0 )
		{
			continue;
		}
#ifdef SIM_OVERLAP
		double sim = (double)(inter)/min(size_step,size_front);
#else
		double sim = (double)(inter)/(size_step+size_front-inter);
#endif
		if( sim > m_threshold )
		{
			matches.push_back( dyn_index );
		}
	}
}

// ------------------------------------------------------------------------------------------
// Class: MapMatchingDynamicClusterer
// ------------------------------------------------------------------------------------------

MapMatchingDynamicClusterer::MapMatchingDynamicClusterer( const double matching_threshold, const int death_age ) 
	: MatchingDynamicClusterer( matching_threshold, death_age )
{
}

bool MapMatchingDynamicClusterer::add_clustering( Clustering &step_clustering )
{
	m_step += 1;
	/// First?
	if( m_step == 1 )
	{
		return bootstrap(step_clustering);
	}
	
	int step_cluster_index = 0;

	/// Build a map of Nodes -> Dynamic Communities containing those nodes
	map<NODE,set<int> > fastmap;
	DynamicClustering::iterator dit;
	DynamicClustering::iterator dend = m_dynamic.end();
	int dyn_count = (int)m_dynamic.size();
	int dyn_index = 0;
	long* dyn_sizes = new long[dyn_count+1];
	for( dit = m_dynamic.begin() ; dit != dend; dit++, dyn_index++ )
	{
		// Dead?
		if( m_death_age > 0 && m_dynamic[dyn_index].is_dead( m_step, m_death_age ) )
		{
			dyn_sizes[dyn_index] = 0;
			continue;
		}
		Cluster& front = (*dit).front();
		dyn_sizes[dyn_index] = (long)front.size();
		Cluster::const_iterator fit;
		Cluster::const_iterator	fend = front.end();
		for( fit = front.begin() ; fit != fend; fit++ )
		{
			NODE node_index = *fit;
			if( !fastmap.count( node_index ) )
			{
				set<int> first;
				first.insert(dyn_index);
				fastmap.insert( make_pair(node_index,first) );
			}
			else
			{
				fastmap[node_index].insert(dyn_index);
			}
		}
	}	

	/// Now try to match all
	int* all_intersection = new int[dyn_count+1];
	vector<DynamicCluster> fresh;
	PairVector matched_pairs;
	map<NODE,set<int> >::const_iterator mend = fastmap.end();
	Clustering::iterator cit;
	Clustering::iterator cend = step_clustering.end();
	for( cit = step_clustering.begin() ; cit != cend; cit++, step_cluster_index++ )
	{
		long size_step = (long)(*cit).size();
		if( size_step < MIN_CLUSTER_SIZE )
		{
			continue;
		}
		// Compute all intersections
		for( dyn_index = 0; dyn_index < dyn_count; dyn_index++)
		{
			all_intersection[dyn_index] = 0;
		}
		Cluster::const_iterator xit;
		Cluster::const_iterator	xend = (*cit).end();
		for( xit = (*cit).begin() ; xit != xend; xit++ )
		{
			NODE node_index = *xit;
			map<NODE,set<int> >::const_iterator mit = fastmap.find(node_index);
			if( mit != mend )
			{
				set<int>::const_iterator sit;
				for ( sit = fastmap[node_index].begin(); sit != fastmap[node_index].end(); sit++ )
				{
					all_intersection[(*sit)]++;
				}
			}
		}
		// Find matches
		vector<int> matches;
		for( dyn_index = 0; dyn_index < dyn_count; dyn_index++)
		{
			if( dyn_sizes[dyn_index] == 0 || all_intersection[dyn_index] == 0 )
			{
				continue;
			}
#ifdef SIM_OVERLAP
			double sim = ((double)(all_intersection[dyn_index]))/min(size_step,dyn_sizes[dyn_index]);
#else
			double sim = ((double)(all_intersection[dyn_index]))/(size_step+dyn_sizes[dyn_index]-all_intersection[dyn_index]);
#endif
			if( sim > m_threshold )
			{
				matches.push_back( dyn_index );
			}
		}

		// new community?
		if( matches.empty() )
		{
			DynamicCluster dc;
			dc.update( m_step, step_cluster_index, *cit );
			fresh.push_back(dc);
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Birth: Community M" << (m_dynamic.size()+fresh.size()) << " from C" << step_cluster_index+1 << endl;
#endif			
		}
		else
		{
			vector<int>::const_iterator iit;
			for( iit = matches.begin() ; iit != matches.end(); iit++ )
			{
				pair<int,int> p(step_cluster_index,(*iit));
				matched_pairs.push_back(p);
			}
		}
	}

	// Actually update existing dynamic communities now
	set<int> matched_dynamic;
	PairVector::const_iterator pit;
	for( pit = matched_pairs.begin(); pit != matched_pairs.end(); pit++ )
	{
		int step_cluster_index = (*pit).first;
		int dyn_cluster_index = (*pit).second;
		// already processed this dynamic cluster?
		if( matched_dynamic.count( dyn_cluster_index ) ) 
		{
			DynamicCluster dc( m_dynamic[dyn_cluster_index], m_step, step_cluster_index, step_clustering[step_cluster_index] );
			fresh.push_back(dc);
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Split: Matched C" << (step_cluster_index+1) << " to M" << (dyn_cluster_index+1) << ". Splitting to M" << (m_dynamic.size()+fresh.size()) <<  endl;
#endif
		}
		else
		{
#ifdef DEBUG_MATCHING
			cout << "T" << m_step << ": Continuation: Matched C" << (step_cluster_index+1) << " to M" << (dyn_cluster_index+1) << endl;
#endif
			m_dynamic[dyn_cluster_index].update( m_step, step_cluster_index, step_clustering[step_cluster_index] );
			matched_dynamic.insert(dyn_cluster_index);
		}
	}
	// And finally add any new dynamic communities
	for( dit = fresh.begin() ; dit != fresh.end(); dit++ )
	{
		m_dynamic.push_back(*dit);
	}

	delete[] dyn_sizes;
	delete[] all_intersection;
	return true;
}


// ------------------------------------------------------------------------------------------
// Utility Functions
// ------------------------------------------------------------------------------------------

int count_dead( const DynamicClustering& dynamic, const int current_step, const int death_age )
{
	int count = 0;
	DynamicClustering::const_iterator dit = dynamic.begin();
	for( dit = dynamic.begin() ; dit != dynamic.end(); dit++ )
	{
		if( (*dit).is_dead( current_step, death_age ) )
		{
			count++;
		}
	}
	return count;
}

ostream& operator<<(ostream& os, const Timeline& timeline)
{
	for( int i = 0; i < timeline.size(); i++ )
	{
		if( i > 0 )
		{
			os << ",";
		}
		os << timeline.m_history_step[i] << "=" << (timeline.m_history_cluster[i]+1);
	}
	return os;
}

void print_dynamic_clustering( DynamicClustering &dynamic )
{
	DynamicClustering::iterator dit = dynamic.begin();
	int dyn_index = 1;
	for( dit = dynamic.begin() ; dit != dynamic.end(); dit++, dyn_index++ )
	{
		cout << "M" << dyn_index << ":" << (*dit) << endl;
	}
}

bool write_timelines( const string fname, const DynamicClustering &dynamic )
{
	ofstream fout(fname.c_str()); 
	if(!fout) 
	{  
    	return false; 
   }
	DynamicClustering::const_iterator dit = dynamic.begin();
	int dyn_index = 1;
	for( dit = dynamic.begin() ; dit != dynamic.end(); dit++, dyn_index++ )
	{
		fout << "M" << dyn_index << ":" << (*dit) << endl;
	}
	fout.close();
	return true;
}

bool read_timelines( const string fname, vector<Timeline>& timelines, int &max_step )
{
	timelines.clear();
	ifstream fin(fname.c_str());
	if(!fin) 
	{  
    	return false; 
   } 
	string line;
	max_step = 0;
	int num = 0;
	size_t found;
	while(getline(fin, line) ) 
	{
		num += 1;
		found = line.find(":");
		if( found == string::npos )
		{
			continue;
		}
		line = line.substr(found+1);
		stringstream ss(line);
		string temp;
		vector<int> steps;
		vector<int> cluster_indices;
	   while (getline(ss, temp, ',')) 
		{  
			found = temp.find("=");
			if( found == string::npos )
			{
				cerr << "Error: unexpected timeline definition on line " << num << endl;
				return false;
			}
			int step;
			stringstream is(temp.substr(0,found));
			if( (is >> step).fail() || step < 1 )
			{
				cerr << "Error: Invalid step index '" << is << "' on line " << num << endl;
				return false;
			}
			int step_cluster_index;
			stringstream ic(temp.substr(found+1));
			if( (ic >> step_cluster_index).fail() || step_cluster_index < 1 )
			{
				cerr << "Error: Invalid cluster index '" << ic << "' on line " << num << endl;
				return false;
			}
			steps.push_back(step);
			cluster_indices.push_back(step_cluster_index);
			max_step = max(max_step, step);
	   }
		if( steps.size() > 0 )
		{
			Timeline timeline( steps, cluster_indices );
			timelines.push_back(timeline);
		}
	}
	if( timelines.empty() )
	{
		cerr << "Error: file contained no valid timelines" << endl;
		return false;
	}
	return true;
}
