#!/bin/bash
set -e

ROOT_DIR=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT_DIR"

echo "Running tests..."

make all

TEST_CORPUS="tests/test_corpus"
rm -rf "$TEST_CORPUS"
mkdir -p "$TEST_CORPUS"

cat > "$TEST_CORPUS/doc0.txt" <<'EOF'
Kernel memory management is a core part of operating system design. Memory allocation and deallocation
must be handled carefully to avoid leaks and race conditions. This document describes the key algorithms
used in kernel memory management including paging, segmentation and slab allocators.
EOF

cat > "$TEST_CORPUS/doc1.txt" <<'EOF'
Network protocols and socket programming provide the building blocks for distributed systems.
Reliable communication is obtained using TCP, while UDP is used for lightweight transfers.
EOF

cat > "$TEST_CORPUS/doc2.txt" <<'EOF'
Filesystems like ext4 and xfs implement journaling to improve reliability. Journaling ensures
that metadata changes are recorded and can be recovered after a crash.
EOF

./index_builder "$TEST_CORPUS" "tests/test_index.bin"

OUTPUT=$(echo "kernel AND memory" | ./search_cli "tests/test_index.bin")
echo "Search output:"
echo "$OUTPUT"

if echo "$OUTPUT" | grep -q "Found 0 documents"; then
    echo "Test failed: query returned 0 documents"
    exit 2
fi

echo "Test passed."
