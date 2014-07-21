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
#include "extras.h"
#include <ctime>

/**
 * Determines whether a cluster falls below the minimum size.
 */
bool is_small( Cluster &cluster )
{
	return cluster.size() < MIN_CLUSTER_SIZE;
}

/**
 * Removes small clusters from the specified clustering.
 */
int remove_small_clusters( Clustering &clustering )
{	
	int previous = (int)clustering.size();
	clustering.erase( remove_if( clustering.begin(), clustering.end(), is_small ), clustering.end() );
	return previous - (int)clustering.size();
}

