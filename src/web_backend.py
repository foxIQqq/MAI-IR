import subprocess
import os
from flask import Flask, request, render_template_string

app = Flask(__name__)

SEARCH_BINARY = "./search_cli"
INDEX_FILE = "dumps/main_index.bin"

search_process = None

def get_search_process():
    global search_process
    if search_process is None or search_process.poll() is not None:
        if not os.path.exists(INDEX_FILE):
             print(f"ERROR: Index file {INDEX_FILE} missing! Run indexer first.")
             return None
             
        search_process = subprocess.Popen(
            [SEARCH_BINARY, INDEX_FILE],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1 
        )
        while True:
            line = search_process.stderr.readline()
            if "Ready" in line:
                break
            if not line:
                break
    return search_process

@app.route("/", methods=["GET", "POST"])
def index():
    results = []
    query = ""
    error_msg = ""
    
    if request.method == "POST":
        query = request.form.get("query", "")
        if query:
            proc = get_search_process()
            if proc:
                try:
                    proc.stdin.write(query + "\n")
                    proc.stdin.flush()
                    
                    output_lines = []
                    while True:
                        line = proc.stdout.readline()
                        if "---END---" in line or not line:
                            break
                        output_lines.append(line.strip())
                    results = output_lines
                except Exception as e:
                    error_msg = f"Search engine error: {e}"
            else:
                error_msg = "Search engine backend is not running. Index missing?"

    html = """
    <!doctype html>
    <html lang="ru">
    <head>
        <meta charset="utf-8">
        <title>Scholar Search</title>
        <style>
            body { font-family: 'Segoe UI', sans-serif; max-width: 800px; margin: 40px auto; padding: 20px; background: #fafafa; }
            h1 { color: #333; }
            .search-box { display: flex; gap: 10px; }
            input[type="text"] { flex-grow: 1; padding: 12px; font-size: 16px; border: 1px solid #ddd; border-radius: 4px; }
            input[type="submit"] { padding: 12px 24px; font-size: 16px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer; }
            input[type="submit"]:hover { background: #0056b3; }
            .result { background: white; margin-bottom: 15px; padding: 15px; border-radius: 4px; box-shadow: 0 1px 3px rgba(0,0,0,0.1); }
            .count { color: #666; margin: 20px 0; }
            .error { color: red; background: #ffe6e6; padding: 10px; }
        </style>
    </head>
    <body>
        <h1>Scholar Search Engine</h1>
        {% if error_msg %}
            <div class="error">{{ error_msg }}</div>
        {% endif %}
        <form method="post" class="search-box">
            <input type="text" name="query" value="{{ query }}" placeholder="Search query (e.g. 'kernel AND memory')...">
            <input type="submit" value="Search">
        </form>
        
        {% if results %}
            {% for res in results %}
                {% if "Found" in res %}
                    <div class="count">{{ res }}</div>
                {% else %}
                    <div class="result">
                        <strong>Document:</strong> {{ res }}
                    </div>
                {% endif %}
            {% endfor %}
        {% endif %}
    </body>
    </html>
    """
    return render_template_string(html, query=query, results=results, error_msg=error_msg)

if __name__ == "__main__":
    app.run(port=5000, debug=False)