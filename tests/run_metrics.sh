#!/bin/bash

python3 scripts/generate_qrels_and_results.py --queries scripts/compare/queries.txt \
    --index dumps/main_index.bin --corpus data/corpus \
    --out-qrels scripts/compare/qrels.txt --out-results scripts/compare/results.txt --topk 100

sleep 1

python3 scripts/eval_metrics.py scripts/compare/qrels.txt scripts/compare/results.txt > scripts/metrics.txt 

echo -e "\nMetrics saved at scripts/metrics.txt"