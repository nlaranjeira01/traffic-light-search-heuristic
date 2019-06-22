#include <iostream>
#include <string>
#include <cstring>

#include "heuristic.h"
#include "benchmark.h"

#define DEFAULT_NUMBER_OF_VERTICES 10
#define DEFAULT_NUMBER_OF_RUNS 10
#define DEFAULT_CYCLE 20

#define WRONG_ARGUMENTS_EXIT_CODE 1

using namespace std;
using namespace traffic;
using namespace benchmark;

void setupExecutionParameters (int argc, char** argv, size_t &numberOfVertices, unsigned &numberOfRuns, TimeUnit &cycle) {
	numberOfVertices = DEFAULT_NUMBER_OF_VERTICES;
	numberOfRuns = DEFAULT_NUMBER_OF_RUNS;
	cycle = DEFAULT_CYCLE;

	if (argc > 1) {
		int i = 1;
		while (i < argc) {
			if (strcmp(argv[i], "--vertices") == 0) {
				i++;
				if (i >= argc) {
					cout << "--vertices argument requires a number greater than 0" << endl;
					exit(WRONG_ARGUMENTS_EXIT_CODE);
				}
				numberOfVertices = atoi(argv[i]);
				if (numberOfVertices == 0) {
					cout << "--vertices argument requires a number greater than 0" << endl;
					exit(WRONG_ARGUMENTS_EXIT_CODE);
				}
			} else if (strcmp(argv[i], "--runs") == 0) {
				i++;
				if (i >= argc) {
					cout << "--runs argument requires a number greater than 0" << endl;
					exit(WRONG_ARGUMENTS_EXIT_CODE);
				}
				numberOfRuns = atoi(argv[i]);
				if (numberOfRuns == 0) {
					cout << "--runs argument requires a number greater than 0" << endl;
					exit(WRONG_ARGUMENTS_EXIT_CODE);
				}

			}
			i++;
		}
	}
}

int main (int argc, char** argv) {

	GraphBuilder *graphBuilder;
	Graph *graph;
	list<Observer*> observers;
	TerminalObserver *terminalObserver;
	size_t numberOfVertices;
	unsigned numberOfRuns;
	double avgRandomVariety, avgHeuristicVariety;
	double avgRandomPenalty, avgHeuristicPenalty;
	TimeUnit cycle;
	Solution *randomSolution, *heuristicSolution;
	list<Solution*> randomSolutions, heuristicSolutions;
	double varietyFactor, penaltyFactor;
	decltype(randomSolutions)::iterator it;
	chrono::nanoseconds avgRandomTime, avgHeuristicTime;
	chrono::high_resolution_clock::time_point beginTime;
	string formatedAvgRandomTime, formatedAvgHeuristicTime;
	double timeFactor;

	setupExecutionParameters(argc, argv, numberOfVertices, numberOfRuns, cycle);

	terminalObserver = new TerminalObserver("initial solution construction", numberOfRuns);
	terminalObserver->observeVariable("Random construction variety", avgRandomVariety);
	terminalObserver->observeVariable("Random construction penalty", avgRandomPenalty);
	terminalObserver->observeVariable("Random construction time", formatedAvgRandomTime);
	terminalObserver->observeVariable("Heuristic construction variety", avgHeuristicVariety);
	terminalObserver->observeVariable("Heuristic construction penalty", avgHeuristicPenalty);
	terminalObserver->observeVariable("Heuristic construction time", formatedAvgHeuristicTime);
	terminalObserver->observeVariable("Heuristic/Random variety factor", varietyFactor);
	terminalObserver->observeVariable("Heuristic/Random penalty factor", penaltyFactor);
	terminalObserver->observeVariable("Heuristic/Random time factor", timeFactor);
	observers.push_back(terminalObserver);

	avgRandomVariety = 0;
	avgHeuristicVariety = 0;
	for (auto o : observers) o->notifyBenchmarkBegun();
	for (unsigned i = 0; i < numberOfRuns; i++) {
		graphBuilder = new GraphBuilder(numberOfVertices, 1, numberOfVertices/3, 1, cycle-1);
		graphBuilder->withCycle(cycle);
		graph = graphBuilder->buildAsAdjacencyList();

		for (auto o : observers) o->notifyRunBegun();

		beginTime = chrono::high_resolution_clock::now();
		constructRandomSolution(*graph);
		if (i == 0) {
			avgRandomTime = chrono::high_resolution_clock::now() - beginTime;
			avgRandomPenalty = graph->totalPenalty();
		} else {
			avgRandomTime = (avgRandomTime + chrono::high_resolution_clock::now() - beginTime)/2;
			avgRandomPenalty = (avgRandomPenalty+graph->totalPenalty())/2;
		}
		formatedAvgRandomTime = format_chrono_duration(avgRandomTime);

		randomSolution = graph->extractSolution();

		if (i > 0) {
			it = randomSolutions.begin();
			if (avgRandomVariety == 0) {
				avgRandomVariety = distance(*graph, **it, *randomSolution);
			} else {
				avgRandomVariety = (avgRandomVariety+distance(*graph, **it, *randomSolution))/2;
			}
			for (it++; it != randomSolutions.end(); it++) {
				avgRandomVariety = (avgRandomVariety+distance(*graph, **it, *randomSolution))/2;
			}
		}
		randomSolutions.push_back(randomSolution);

		beginTime = chrono::high_resolution_clock::now();
		constructHeuristicSolution(*graph);
		if (i == 0) {
			avgHeuristicTime = chrono::high_resolution_clock::now() - beginTime;
			avgHeuristicPenalty = graph->totalPenalty();
		} else {
			avgHeuristicTime = (avgHeuristicTime + chrono::high_resolution_clock::now() - beginTime)/2;
			avgHeuristicPenalty = (avgHeuristicPenalty + graph->totalPenalty())/2;
		}
		formatedAvgHeuristicTime = format_chrono_duration(avgHeuristicTime);

		heuristicSolution = graph->extractSolution();

		if (i > 0) {
			it = heuristicSolutions.begin();
			if (avgHeuristicVariety == 0) {
				avgHeuristicVariety = distance(*graph, **it, *heuristicSolution);
			} else {
				avgHeuristicVariety = avgHeuristicVariety+distance(*graph, **it, *heuristicSolution);
			}
			for (it++; it != heuristicSolutions.end(); it++) {
				avgHeuristicVariety = (avgHeuristicVariety+distance(*graph, **it, *heuristicSolution))/2;
			}
		}
		heuristicSolutions.push_back(heuristicSolution);

		varietyFactor = (double)avgHeuristicVariety/avgRandomVariety;
		penaltyFactor = (double)avgHeuristicPenalty/avgRandomPenalty;
		timeFactor = (double)avgHeuristicTime.count()/avgRandomTime.count();

		for (auto o : observers) {
			o->notifyRunUpdate();
			o->notifyRunEnded();
		}

		delete graph;
		delete graphBuilder;
	}
	for (auto o : observers) o->notifyBenchmarkEnded();

	delete terminalObserver;
	for (auto solution : randomSolutions) {
		delete solution;
	}
	for (auto solution : heuristicSolutions) {
		delete solution;
	}

	return 0;
}