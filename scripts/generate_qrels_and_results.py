import argparse
import subprocess
import os
import re
from collections import defaultdict

TOKEN_RE = re.compile(r'\w+', flags=re.UNICODE)

def tokenize_text(s):
    return TOKEN_RE.findall(s.lower())

def count_matches(tokens_query, text):
    tokens = tokenize_text(text)
    if not tokens:
        return 0
    freq = {}
    for t in tokens:
        freq[t] = freq.get(t, 0) + 1
    total = 0
    for q in tokens_query:
        total += freq.get(q, 0)
    return total

def run_search_cli(index_path, query, topk):
    """
    Run ./search_cli [index_path] and send a single query line, parse output
    Return list of docnames in returned order (up to topk)
    """
    cmd = ['./search_cli', index_path] if index_path else ['./search_cli']
    try:
        proc = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    except FileNotFoundError as e:
        raise RuntimeError(f"search_cli not found at {cmd[0]} (cwd={os.getcwd()}). Build project first.") from e

    proc.stdin.write(query + "\n")
    proc.stdin.flush()

    docs = []
    while True:
        line = proc.stdout.readline()
        if not line:
            break
        line = line.rstrip("\n").rstrip("\r")
        if line.strip() == '---END---':
            break
        if line.startswith("Found "):
            continue
        if line.startswith("... and"):
            break
        if line.strip() == "":
            continue
        docs.append(line.strip())
        if len(docs) >= topk:
            pass
    try:
        proc.stdin.close()
    except Exception:
        pass
    try:
        proc.stdout.read()
    except Exception:
        pass
    proc.terminate()
    return docs[:topk]

def build_qrels_for_query(query_id, query_str, corpus_dir, rel2_thr, rel1_thr):
    q_tokens = [t for t in tokenize_text(query_str) if t]
    if not q_tokens:
        return []
    qrels = []
    for fname in sorted(os.listdir(corpus_dir)):
        if not fname.lower().endswith('.txt'):
            continue
        path = os.path.join(corpus_dir, fname)
        try:
            with open(path, 'r', encoding='utf-8', errors='ignore') as f:
                txt = f.read()
        except Exception:
            continue
        cnt = count_matches(q_tokens, txt)
        if cnt >= rel2_thr:
            qrels.append((fname, 2))
        elif cnt >= rel1_thr:
            qrels.append((fname, 1))
    return qrels

def main():
    p = argparse.ArgumentParser()
    p.add_argument('--queries', required=True, help='File with queries, one per line. Optionally: id<TAB>query')
    p.add_argument('--index', default='dumps/main_index.bin', help='Path to index file (default: dumps/main_index.bin)')
    p.add_argument('--corpus', default='data/corpus', help='Path to corpus directory with .txt files (default: data/corpus)')
    p.add_argument('--out-qrels', default='qrels.txt', help='Output qrels file (default: qrels.txt)')
    p.add_argument('--out-results', default='results.txt', help='Output results file (default: results.txt)')
    p.add_argument('--topk', type=int, default=100, help='How many results to collect per query (default 100)')
    p.add_argument('--rel2-threshold', type=int, default=5, help='>= this -> relevance 2 (default 5)')
    p.add_argument('--rel1-threshold', type=int, default=1, help='>= this -> relevance 1 (default 1)')
    args = p.parse_args()

    if not os.path.exists(args.queries):
        print("Queries file not found:", args.queries)
        return
    if not os.path.exists(args.index):
        print("Warning: index file not found:", args.index)
    if not os.path.isdir(args.corpus):
        print("Corpus directory not found:", args.corpus)
        return

    queries = []
    with open(args.queries, 'r', encoding='utf-8') as f:
        for i,line in enumerate(f):
            line=line.strip()
            if not line: continue
            if '\t' in line:
                qid, q = line.split('\t',1)
            elif ' ' in line and line.split()[0].startswith('q'):
                t = line.split(None,1)
                if len(t) == 2 and t[0].startswith('q'):
                    qid, q = t[0], t[1]
                else:
                    qid, q = f"q{i+1}", line
            else:
                qid = f"q{i+1}"
                q = line
            queries.append((qid, q))

    out_qrels = open(args.out_qrels, 'w', encoding='utf-8')
    out_results = open(args.out_results, 'w', encoding='utf-8')

    for qid, qtext in queries:
        print(f"Processing {qid}: {qtext}")
        try:
            docs = run_search_cli(args.index, qtext, args.topk)
        except Exception as e:
            print("Error running search_cli:", e)
            docs = []
        for rank, doc in enumerate(docs, start=1):
            out_results.write(f"{qid} {doc} {rank}\n")

        qrels = build_qrels_for_query(qid, qtext, args.corpus, args.rel2_threshold, args.rel1_threshold)
        for docname, rel in qrels:
            out_qrels.write(f"{qid} {docname} {rel}\n")

    out_qrels.close()
    out_results.close()
    print("Done. Wrote:", args.out_qrels, args.out_results)

if __name__ == "__main__":
    main()
