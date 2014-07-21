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

#ifndef CLUSTER_H
#define CLUSTER_H

/** represents an individual cluster */
typedef set<NODE> Cluster;
/** represents a vector of zero or more clusters */
typedef vector<Cluster> Clustering;

long assigned( const Clustering &clustering, set<NODE> &nodes );
long assigned_count( const Clustering &clustering );
long overlapping_count( const Clustering &clustering );
long max_cluster_size( const Clustering &clustering );
int count_empty_clusters( const Clustering &clustering );

int remove_small_clusters( Clustering &clustering, const int min_size );
int remove_duplicate_clusters( Clustering &clustering );

bool read_clustering( const string fname, const char sep, Clustering &clustering);
bool write_clustering( const string fname, const char sep, const Clustering &clustering );
void print_cluster( Cluster &cluster );
void print_cluster_sizes( const Clustering &clustering );

#endif // CLUSTER_H
