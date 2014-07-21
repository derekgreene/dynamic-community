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

#ifndef DYNAMIC_H
#define DYNAMIC_H

#include "common/clustering.h"

// ------------------------------------------------------------------------------------------
// Class: Timeline
// ------------------------------------------------------------------------------------------

class Timeline
{
	public:
		Timeline( vector<int> &history_step, vector<int> &history_cluster );
		Timeline() {};
		~Timeline() {};

		int last_observed() const;
		int first_observed() const;
		int size() const;
		int consecutive_length() const;
		
		bool is_observed( const int step ) const; 	
		bool is_dead( const int step, const int death_age ) const;
		// int get_cluster_index( const int step ) const; 	

 		int operator[] ( const int step ) const;   
		friend ostream& operator<<(ostream& os, const Timeline& dt);

	protected:
		vector<int> m_history_step;
		vector<int> m_history_cluster; 
};

// ------------------------------------------------------------------------------------------
// Class: DynamicCluster
// ------------------------------------------------------------------------------------------

class DynamicCluster : public Timeline
{
public:
	DynamicCluster() : Timeline() {};
	DynamicCluster( DynamicCluster &sibling, const int step, const int step_cluster_index, Cluster &initial_front );
	~DynamicCluster() {};

	bool update( const int step, const int step_cluster_index, Cluster &initial_front );
	Cluster &front();

protected:
	Cluster m_front;
};

typedef vector<DynamicCluster> DynamicClustering;
typedef vector<pair<int,int> > PairVector;

// ------------------------------------------------------------------------------------------
// Class: MatchingDynamicClusterer
// ------------------------------------------------------------------------------------------

class MatchingDynamicClusterer
{
public:
	MatchingDynamicClusterer( const double matching_threshold, const int death_age );
	
	virtual bool add_clustering( Clustering &step_clustering );
	DynamicClustering &find_clusters();
	
protected:
	virtual void find_matches( const Cluster &step_cluster, vector<int> &matches );
	bool bootstrap( Clustering &step_clustering );
	
	/** matching threshold */
	double m_threshold;
	/** age at which communities die if not observed */
	int m_death_age;
	/** set of dynamic clusters */
	DynamicClustering m_dynamic;
	/** current step number */
	int m_step;
};

// ------------------------------------------------------------------------------------------
// Class: MapMatchingDynamicClusterer
// ------------------------------------------------------------------------------------------

class MapMatchingDynamicClusterer : public MatchingDynamicClusterer
{
public:
	MapMatchingDynamicClusterer( const double matching_threshold, const int death_age );
	
	virtual bool add_clustering( Clustering &step_clustering );
};

// ------------------------------------------------------------------------------------------

int count_dead( const DynamicClustering& dynamic, const int current_step, const int death_age );
bool read_timelines( const string fname, vector<Timeline>& timelines, int &max_step );
bool write_timelines( const string fname, const DynamicClustering &dynamic );
void print_dynamic_clustering( DynamicClustering &dynamic );

// ------------------------------------------------------------------------------------------

#endif // DYNAMIC_H
