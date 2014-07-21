dynamic-community
=================

_Dynamic Community Tracking Tool_
Derek Greene, Insight Centre @ University College Dublin
### Overview

The Dynamic Community Tracking Tool is a console application written in C++ for identifying and tracking communities of nodes in dynamic networks, where these networks are represented as a set of step graphs representing snapshots of the network at successive time periods.
The tool takes as its input a series of two or more sets of step communities, produced on individual time step graphs using a standard static community finding algorithm, such as [MOSES](https://sites.google.com/site/aaronmcdaid/moses) or the [Louvain algorithm](http://sites.google.com/site/findcommunities). These step communities can overlapping or non-overlapping. Note that all nodes need be present in consecutive time step graphs, some degree of overlap is sufficient.
The tool builds dynamic community timelines from sequences of individual step communities, which can be used to chart the evolution of these dynamic communities over time.
For further details on the algorithm and its applications, please consult the following paper:
- D.Greene, D.Doyle, and P.Cunningham, "Tracking the evolution of communities in dynamic social networks," in Proc. International Conference on Advances in Social Networks Analysis and Mining (ASONAM'10), 2010. [[PDF]](http://mlg.ucd.ie/files/publications/greene10tracking.pdf)

### Usage

The main tracker tool is run from the command line as follows:

	./tracker [matching_threshold] [output_prefix] step1_communities step2_communities ...

The parameter *matching_threshold* is a value [0,1] indicating the threshold required to match communi- ties between time steps. A higher value indicates a more conservative matching threshold. Low values are suitable for data where community memberships are expected to be transient over time, high values are suitable where community memberships are expected to be consistent over time.
The parameter *output_prefix* provides a string that is added as a prefix to the output files produced by the tool.
The subsequent parameters correspond to a list of paths of input files containing step communities, with one file per step. The first file is assumed to correspond to the first time step, the second file to the second time step, and so on. The format for the input files is given in the next section.
For example, to apply the tool to 3 sets of step communities, with a matching threshold of 0.3 and output prefix of "res":
	./tracker 0.3 res sample/sample.t01.comm sample/sample.t02.comm sample/sample.t03.comm
### Input Format
Each plain text input file for the tracker tool contains one or more step communities, with one line corresponding to each community. The entries on each line correspond to the node identifiers (positive numeric values) separated by spaces. Note that node identifier numbers need not be consecutive, or ordered in the file.Below shows a simple example of an input file containing three overlapping communities:	1 2 3 10 4 
	5 3 6 7 8 9
	10 11 12 1 4### Output FormatThe output of the tracker tool is a single file, where each line in the file correspond to the timeline of a dynamic community. The entries in each line correspond to the sequence of associated step community observations which form that dynamic community.Below shows a simple example of an output file containing two dynamic communities over three time steps:
	￼￼￼M1:1=1,2=2,3=1
	M2:2=2,3=1
For example, in the case of the second dynamic community (named "M2"), the dynamic community was not observed at t = 1, and consists of the 2nd step community at time t = 2, and the 1st step community at time t = 3. These step community indices correspond to the line numbers in the original input files supplied to the tracker tool.
To produce a set of dynamic communities in the same format as the input file, the union tool is run from the command line as follows:
	./union [output_prefix] [max_step] [timeline_file] step1_communities step2_communities ...The parameter *output_prefix* provides a string that is added as a prefix to the output community files produced by the tool.
The parameter *max_step* indicates the maximum step number for which communities should be included. Typically this should correspond to the number of step community files specified.The parameter *timeline_file* corresponds to the name of the output file from the tracker tool.