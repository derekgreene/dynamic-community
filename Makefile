CC=g++
CFLAGS=-O3 -funroll-loops -I.
PREFIX=~/bin
DEPS = common/clustering.h common/util.h dynamic.h extras.h settings.h 
OBJ = common/clustering.o common/util.o dynamic.o extras.o  
EXECS = tracker aggregator timeline_stats step_stats aggregator_stats node_stats
ARG_GEN=gengetopt

%.o: %.cpp $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

tracker: $(OBJ) trackerargs.o tracker.o 
	$(CC) -o $@ $^ $(CFLAGS)

aggregator: $(OBJ) aggregatorargs.o aggregator.o
	$(CC) -o $@ $^ $(CFLAGS)

timeline_stats: $(OBJ) timeline_stats.o
	$(CC) -o $@ $^ $(CFLAGS)

step_stats: $(OBJ) step_stats.o
	$(CC) -o $@ $^ $(CFLAGS)

aggregator_stats: $(OBJ) aggregator_statsargs.o aggregator_stats.o
	$(CC) -o $@ $^ $(CFLAGS)	

node_stats: $(OBJ) node_stats.o
	$(CC) -o $@ $^ $(CFLAGS)	

all: tracker aggregator timeline_stats step_stats aggregator_stats node_stats

args: tracker.ggo aggregator.ggo
	$(ARG_GEN) -i tracker.ggo -a tracker_args_info -F trackerargs --unamed-opts=STEP_COMMUNITIES
	$(ARG_GEN) -i aggregator.ggo -a aggregator_args_info -F aggregatorargs --unamed-opts=STEP_COMMUNITIES
	$(ARG_GEN) -i aggregator_stats.ggo -a aggregator_stats_args_info -F aggregator_statsargs --unamed-opts=STEP_COMMUNITIES

clean:
	rm -f *.o common/*.o *~ $(EXECS)

install:
	cp tracker $(PREFIX)
	cp aggregator $(PREFIX)
	cp timeline_stats $(PREFIX)
	cp step_stats $(PREFIX)
	cp aggregator_stats $(PREFIX)
	cp node_stats $(PREFIX)

