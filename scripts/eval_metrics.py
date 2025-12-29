import sys
import math
from collections import defaultdict

def read_qrels(path):
    qrels = defaultdict(dict)
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 3: continue
            qid, doc, rel = parts[0], parts[1], float(parts[2])
            qrels[qid][doc] = rel
    return qrels

def read_results(path):
    res = defaultdict(list)
    with open(path, 'r', encoding='utf-8') as f:
        for line in f:
            parts = line.strip().split()
            if len(parts) < 3: continue
            qid, doc, rank = parts[0], parts[1], int(parts[2])
            res[qid].append((rank, doc))
    for q in res:
        res[q].sort()
        res[q] = [d for _,d in res[q]]
    return res

def precision_at_k(rels, k):
    if k == 0: return 0.0
    return sum(1.0 for r in rels[:k] if r>0)/float(k)

def dcg_at_k(rels, k):
    dcg = 0.0
    for i in range(min(k,len(rels))):
        num = (2**rels[i] - 1)
        denom = math.log2(i+2)
        dcg += num/denom
    return dcg

def ndcg_at_k(rels, k):
    ideal = sorted(rels, reverse=True)
    idcg = dcg_at_k(ideal, k)
    if idcg == 0: return 0.0
    return dcg_at_k(rels, k)/idcg

def err_at_k(rels, k):
    err = 0.0
    p = 1.0
    for i in range(min(k, len(rels))):
        R = (2**rels[i]-1)/(2**max(rels) if len(rels)>0 else 1)
        err += p * R / (i+1)
        p *= (1 - R)
    return err

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: eval_metrics.py qrels.txt results.txt")
        sys.exit(1)
    qrels = read_qrels(sys.argv[1])
    results = read_results(sys.argv[2])
    Ks = [1,5,10,20]
    for q in results:
        rels = [ qrels.get(q, {}).get(doc, 0.0) for doc in results[q] ]
        print("Query:", q)
        for k in Ks:
            print(f" P@{k}: {precision_at_k(rels,k):.4f}  NDCG@{k}: {ndcg_at_k(rels,k):.4f}  ERR@{k}: {err_at_k(rels,k):.4f}")
        print()
