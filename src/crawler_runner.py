#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import time
import requests
from bs4 import BeautifulSoup
import re
import urllib.parse
import urllib.robotparser
from collections import deque

OUTPUT_DIR = "data/corpus"
MIN_DOCS = 30005       # целевое число документов (настраиваемо)
MIN_WORDS = 1000       # минимальное число слов в документе (настраиваемо)
SEEDS_FILE = "seeds.txt"
USER_AGENT = "ScholarIRBot/1.0"
REQUEST_DELAY = 1.0    # сек между запросами

def ensure_dir(d):
    if not os.path.exists(d):
        os.makedirs(d, exist_ok=True)

def save_document(doc_id, title, content):  
    filename = os.path.join(OUTPUT_DIR, f"doc_{doc_id}.txt")
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"{title}\n\n{content}")

def clean_text(text):
    s = re.sub(r'\s+', ' ', text)
    s = s.strip()
    return s

def domain_of(url):
    parsed = urllib.parse.urlparse(url)
    return parsed.scheme + "://" + parsed.netloc

def can_fetch(url, rp_cache):
    parsed = urllib.parse.urlparse(url)
    base = parsed.scheme + "://" + parsed.netloc
    if base not in rp_cache:
        rp = urllib.robotparser.RobotFileParser()
        robots_url = urllib.parse.urljoin(base, "/robots.txt")
        try:
            rp.set_url(robots_url)
            rp.read()
        except Exception:
            rp = None
        rp_cache[base] = rp
    rp = rp_cache[base]
    if rp is None:
        return True
    return rp.can_fetch(USER_AGENT, url)

def extract_links(html, base_url):
    soup = BeautifulSoup(html, "html.parser")
    links = []
    for a in soup.find_all("a", href=True):
        href = a['href']
        url = urllib.parse.urljoin(base_url, href)
        url = url.split('#')[0]
        links.append(url)
    return links

def text_from_html(html):
    soup = BeautifulSoup(html, "html.parser")
    main = soup.find('article') or soup.find('main') or soup.find('div', {"class": "content"}) or soup.body
    if main is None:
        return ""
    return clean_text(main.get_text(separator=' '))

def crawler():
    ensure_dir(OUTPUT_DIR)
    if not os.path.exists(SEEDS_FILE):
        print("Error: seeds.txt not found. Put one seed URL per line in seeds.txt")
        return

    with open(SEEDS_FILE, "r", encoding="utf-8") as f:
        seeds = [line.strip() for line in f if line.strip()]

    session = requests.Session()
    session.headers.update({"User-Agent": USER_AGENT})

    rp_cache = {}
    visited = set()
    q = deque()
    for s in seeds:
        q.append(s)

    docs_collected = 0
    doc_id = 0

    print(f"Starting crawler. Target docs: {MIN_DOCS}. Min words per doc: {MIN_WORDS}")

    while q and docs_collected < MIN_DOCS:
        url = q.popleft()
        if url in visited:
            continue
        visited.add(url)

        if not can_fetch(url, rp_cache):
            continue

        try:
            resp = session.get(url, timeout=10)
        except Exception as e:
            print(f"Request failed {url}: {e}")
            time.sleep(REQUEST_DELAY)
            continue

        time.sleep(REQUEST_DELAY)

        if resp.status_code != 200:
            continue
        content_type = resp.headers.get("Content-Type", "")
        if "text/html" not in content_type:
            continue

        html = resp.text
        text = text_from_html(html)
        words = len(text.split())
        if words >= MIN_WORDS:
            soup = BeautifulSoup(html, "html.parser")
            title_tag = soup.find('title')
            title = clean_text(title_tag.text) if title_tag else url
            save_document(doc_id, title, text)
            doc_id += 1
            docs_collected += 1
            if docs_collected % 100 == 0:
                print(f"Collected {docs_collected}/{MIN_DOCS}")
        try:
            base = domain_of(url)
            links = extract_links(html, base)
            for link in links:
                if not link.startswith("http://") and not link.startswith("https://"):
                    continue
                parsed_seed = urllib.parse.urlparse(seeds[0])
                seed_netloc = parsed_seed.netloc
                if urllib.parse.urlparse(link).netloc.endswith(seed_netloc):
                    if link not in visited:
                        q.append(link)
        except Exception:
            pass

    print(f"Crawling finished. Collected {docs_collected} documents in {OUTPUT_DIR}")

if __name__ == "__main__":
    crawler()
