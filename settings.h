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

#ifndef SETTINGS_H
#define SETTINGS_H

typedef long NODE;

//----------------------------------
// General clustering settings
//----------------------------------

#define MIN_CLUSTER_SIZE 3
//#define MAX_CLUSTER_SIZE 1000

//----------------------------------
// Matching settings
//----------------------------------

#define MAP_MATCHING 1
#define DEFAULT_DEATH_AGE 3
#define LONG_LIVED 2
#define MIN_PERSIST_LENGTH 2
//#define SIM_OVERLAP 1
//#define DEBUG_MATCHING 1
//#define DEBUG_DYNAMIC 1

//----------------------------------
// Miscellaneous Settings
//----------------------------------

#define DEFAULT_DELIM ' '
#define ENABLE_WRITING 1 
//#define DEBUG_CLUSTERING 1

#endif // SETTINGS_H
