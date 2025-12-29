CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -Iinclude -Wno-unused-result

INDEXER = index_builder
SEARCHER = search_cli

.PHONY: all index_main test zipf_plot clean

all: $(INDEXER) $(SEARCHER)

$(INDEXER): src/index_builder.cpp include/sch_containers.h include/sch_string.h include/sch_string_utils.h
	$(CXX) $(CXXFLAGS) -o $(INDEXER) src/index_builder.cpp

$(SEARCHER): src/search_cli.cpp include/sch_containers.h include/sch_string.h include/sch_string_utils.h
	$(CXX) $(CXXFLAGS) -o $(SEARCHER) src/search_cli.cpp

index_main: $(INDEXER)
	mkdir -p dumps
	./$(INDEXER) data/corpus dumps/main_index.bin

test: all
	chmod +x tests/run_test.sh
	bash tests/run_test.sh

zipf_plot:
	@if [ -f dumps/main_index.bin.csv ]; then \
		python3 src/visualize_zipf.py dumps/main_index.bin.csv; \
	else \
		echo "CSV not found: dumps/main_index.bin.csv. Run indexer first."; \
	fi

clean:
	rm -f $(INDEXER) $(SEARCHER)
	rm -rf dumps
	rm -rf tests/test_corpus
	mkdir dumps/
