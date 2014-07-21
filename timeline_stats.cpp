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
		cerr << "Usage: " << argv[0] << " [timeline_file]" << endl;
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
	cout << "Found " << timelines.size() << " dynamic community timelines from " << max_step << " time steps" << endl;

	int *freq = new int[max_step+1];
	int *consec = new int[max_step+1];
	for( int i = 0; i < max_step+1; i++)
	{
		freq[i] = 0;
		consec[i] = 0;
	}

	vector<Timeline>::const_iterator cit = timelines.begin();
	int long_lived = 0;
	int intermittent = 0;
	int dead = 0;
	for( cit = timelines.begin() ; cit != timelines.end(); cit++)
	{
		int seen = (int)((*cit).size());
		if( seen > LONG_LIVED )
		{
			long_lived++;
		}
		int seenConsec = ((*cit).consecutive_length());
		freq[seen]++;
		consec[seenConsec]++;
		if( seen < max_step && (*cit).last_observed() - (*cit).first_observed() > 1 )
		{
			intermittent++;
		}
		if( (*cit).is_dead( max_step, DEFAULT_DEATH_AGE ) )
		{
			dead++;
		}
	}

	double frac_long_lived = 100*(((double)long_lived)/(int)(timelines.size()));
	printf("Observed %d long-lived communities of length >= %d (%.1f%%).\n", long_lived, LONG_LIVED, frac_long_lived );

	int short_lived = (int)(timelines.size()) - long_lived;
	double frac_short_lived = 100*(((double)short_lived)/(int)(timelines.size()));
	printf("Observed %d short-lived communities of length < %d (%.1f%%).\n", short_lived, LONG_LIVED, frac_short_lived );
	
	double frac_intermittent = 100*(((double)intermittent)/(int)(timelines.size()));
	printf("Observed %d intermittent communities (%.1f%%).\n", intermittent, frac_intermittent );
	
	double frac_dead = 100*(((double)dead)/(int)(timelines.size()));
	printf("%d communities were dead by step %d (%.1f%%).\n", dead, max_step, frac_dead );
	
	cout << "Observation frequencies:" << endl;
	for( int i = max_step; i > 0; i--)
	{
		double frac = 100*(((double)freq[i])/(int)(timelines.size()));
		printf( "  Present in %d step(s): %d communities (%.1f%%)\n", i, freq[i], frac );
	}
	cout << "Consecutive observation frequencies:" << endl;
	for( int i = max_step; i > 0; i--)
	{
		double frac = 100*(((double)consec[i])/(int)(timelines.size()));
		printf( "  Present in %d consecutive step(s): %d communities (%.1f%%)\n", i, consec[i], frac );
	}
	
	cout << "Done." << endl;
	
	delete[] freq;
	return 0;
}
