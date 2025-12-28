import matplotlib.pyplot as plt
import csv
import sys
import math

def plot_zipf(csv_file):
    ranks = []
    frequencies = []
    
    try:
        with open(csv_file, 'r') as f:
            reader = csv.reader(f)
            next(reader)
            data = []
            for row in reader:
                if len(row) >= 2:
                    data.append((row[0], int(row[1])))
            
            # Сортировка по убыванию частоты (обязательно для Ципфа)
            data.sort(key=lambda x: x[1], reverse=True)
            
            for rank, (term, freq) in enumerate(data, 1):
                ranks.append(rank)
                frequencies.append(freq)
                
    except FileNotFoundError:
        print(f"Error: File {csv_file} not found.")
        return

    plt.figure(figsize=(10, 6))
    plt.loglog(ranks, frequencies, marker='.', linestyle='none', markersize=2)
    plt.title("Zipf's Law: Frequency vs Rank (Log-Log Scale)")
    plt.xlabel("Rank (log)")
    plt.ylabel("Frequency (log)")
    plt.grid(True, which="both", ls="-", alpha=0.5)
    
    output_img = csv_file.replace(".csv", ".png")
    plt.savefig(output_img)
    print(f"Graph saved to {output_img}")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 visualize_zipf.py <path_to_csv>")
    else:
        plot_zipf(sys.argv[1])