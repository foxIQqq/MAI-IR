import os
import time
import requests
from bs4 import BeautifulSoup
import re

OUTPUT_DIR = "data/corpus"
MIN_DOCS = 30005
BASE_URL = "https://www.opennet.ru"
START_PAGE = "https://www.opennet.ru/opennews/"

def ensure_dir(directory):
    if not os.path.exists(directory):
        os.makedirs(directory)

def save_document(doc_id, title, content):
    filename = os.path.join(OUTPUT_DIR, f"doc_{doc_id}.txt")
    with open(filename, "w", encoding="utf-8") as f:
        f.write(f"{title}\n\n{content}")

def clean_text(text):
    # Убираем лишние пробелы и мусор
    return re.sub(r'\s+', ' ', text).strip()

def crawler():
    ensure_dir(OUTPUT_DIR)
    
    current_id = 60000
    docs_collected = 0
    
    print(f"Starting crawler. Target: {MIN_DOCS} documents.")
    
    session = requests.Session()
    session.headers.update({
        'User-Agent': 'Mozilla/5.0 (Educational IR Lab Bot; Student Project)'
    })

    while docs_collected < MIN_DOCS:
        url = f"{BASE_URL}/opennews/art.shtml?num={current_id}"
        try:
            resp = session.get(url, timeout=5)
            if resp.status_code == 200:
                resp.encoding = 'koi8-r' 
                
                soup = BeautifulSoup(resp.text, 'html.parser')
                
                title_tag = soup.find('title')
                if not title_tag:
                    current_id -= 1
                    continue
                    
                title = clean_text(title_tag.text)
                
                body_tag = soup.find('td', class_='chtext')
                if body_tag:
                    text_content = clean_text(body_tag.get_text())
                    
                    if len(text_content.split()) > 50:
                        save_document(current_id, title, text_content)
                        docs_collected += 1
                        if docs_collected % 100 == 0:
                            print(f"Collected: {docs_collected}/{MIN_DOCS}")
            
        except Exception as e:
            print(f"Error fetching {current_id}: {e}")
            time.sleep(1)

        current_id -= 1
        
        if current_id < 1:
            break

if __name__ == "__main__":
    crawler()