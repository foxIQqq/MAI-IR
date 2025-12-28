CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude

INDEXER = index_builder
SEARCHER = search_cli

all: $(INDEXER) $(SEARCHER)

$(INDEXER): src/index_builder.cpp include/sch_containers.h include/sch_string_utils.h
	$(CXX) $(CXXFLAGS) -o $(INDEXER) src/index_builder.cpp

$(SEARCHER): src/search_cli.cpp include/sch_containers.h include/sch_string_utils.h
	$(CXX) $(CXXFLAGS) -o $(SEARCHER) src/search_cli.cpp

index_main: $(INDEXER)
	mkdir -p dumps
	./$(INDEXER) data/corpus dumps/main_index.bin

test: all
	bash tests/run_tests.sh

zipf_plot:
	python3 src/visualize_zipf.py dumps/main_index.bin.csv

clean:
	rm -f $(INDEXER) $(SEARCHER)
	rm -rf tests/test_corpus tests/test_index.bin* tests/out_*.txt