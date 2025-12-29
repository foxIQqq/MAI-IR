import subprocess
import os
import time
from flask import Flask, request, render_template_string

app = Flask(__name__)

SEARCH_BINARY = "./search_cli"
INDEX_FILE = "dumps/main_index.bin"

search_process = None

def get_search_process():
    global search_process
    if search_process is None or search_process.poll() is not None:
        if not os.path.exists(INDEX_FILE):
             return None
        search_process = subprocess.Popen(
            [SEARCH_BINARY, INDEX_FILE],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1
        )
        timeout = 5.0
        start = time.time()
        while time.time() - start < timeout:
            line = search_process.stderr.readline()
            if not line:
                time.sleep(0.05)
                continue
            if "Ready" in line or "Index loaded" in line:
                break
    return search_process

@app.route("/", methods=["GET", "POST"])
def index():
    results = []
    found_count = "0"
    query = ""
    error_msg = ""

    if request.method == "POST":
        query = request.form.get("query", "").strip()
        if query:
            proc = get_search_process()
            if proc is None:
                error_msg = f"Index file not found ({INDEX_FILE}); запустите индексер."
            else:
                try:
                    proc.stdin.write(query + "\n")
                    proc.stdin.flush()

                    while True:
                        line = proc.stdout.readline()
                        if not line:
                            break
                        line = line.rstrip("\n").rstrip("\r")
                        if line.strip() == "":
                            continue
                        if line.strip() == "---END---":
                            break
                        if line.startswith("Found "):
                            parts = line.split()
                            if len(parts) >= 2:
                                try:
                                    found_count = parts[1]
                                except Exception:
                                    pass
                        else:
                            results.append(line.strip())
                except Exception as e:
                    error_msg = f"Ошибка запуска search_cli: {e}"

    html = """
    <!doctype html>
    <html>
    <head>
        <meta charset="utf-8">
        <title>Scholar Search</title>
        <style>
            body { font-family: sans-serif; max-width: 900px; margin: 30px auto; background: #f4f4f9; color: #333; }
            .search-header { background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 5px rgba(0,0,0,0.1); }
            input[type="text"] { width: 70%; padding: 10px; border: 1px solid #ddd; border-radius: 4px; }
            input[type="submit"] { padding: 10px 20px; background: #28a745; color: white; border: none; border-radius: 4px; cursor: pointer; }
            .results-container { 
                margin-top: 20px; 
                background: white; 
                border-radius: 8px; 
                box-shadow: 0 2px 5px rgba(0,0,0,0.1);
                max-height: 600px; 
                overflow-y: auto;  
                padding: 20px;
            }
            .result-item { 
                padding: 10px; 
                border-bottom: 1px solid #eee; 
                display: flex;
                align-items: center;
            }
            .result-item:last-child { border-bottom: none; }
            .doc-icon { margin-right: 15px; color: #666; font-size: 1.2em; }
            .stats { color: #666; font-size: 0.9em; margin-bottom: 10px; }
            .error { color: #900; background: #ffecec; padding: 10px; border-radius: 4px; margin-bottom: 10px; }
        </style>
    </head>
    <body>
        <div class="search-header">
            <h1>Scholar IR Engine</h1>
            {% if error_msg %}
                <div class="error">{{ error_msg }}</div>
            {% endif %}
            <form method="post">
                <input type="text" name="query" value="{{ query }}" placeholder="Query (e.g. kernel AND linux)">
                <input type="submit" value="Search">
            </form>
        </div>

        {% if query %}
        <div class="results-container">
            <div class="stats">Found documents: {{ found_count }}</div>
            {% if results %}
                {% for res in results %}
                    <div class="result-item">
                        <span class="doc-icon">📄</span>
                        <span>{{ res }}</span>
                    </div>
                {% endfor %}
            {% else %}
                <p>No documents found for this query.</p>
            {% endif %}
        </div>
        {% endif %}
    </body>
    </html>
    """
    return render_template_string(html, query=query, results=results, found_count=found_count, error_msg=error_msg)

if __name__ == "__main__":
    app.run(port=5000)
