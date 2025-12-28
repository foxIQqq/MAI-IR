#!/bin/bash

GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

echo "=== STARTING COMPREHENSIVE TEST SUITE ==="

# 1. Компиляция
echo "[1] Compiling project..."
make clean > /dev/null
make all > /dev/null
if [ $? -ne 0 ]; then
    echo -e "${RED}[FAIL] Compilation failed.${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Compilation successful.${NC}"

# 2. Подготовка тестовых данных
echo "[2] Preparing test data..."
TEST_DIR="tests/test_corpus"
TEST_INDEX="tests/test_index.bin"
rm -rf $TEST_DIR
mkdir -p $TEST_DIR

# Создаем файлы с учетом проверки стемминга
# "Apples" должно превратиться в "appl"
# "running" должно найтись по "run"
echo "I like Apples and running." > $TEST_DIR/doc1.txt
echo "Banana is yellow. Apple is red." > $TEST_DIR/doc2.txt
echo "Run fast." > $TEST_DIR/doc3.txt

# 3. Индексация
echo "[3] Indexing test corpus..."
./index_builder $TEST_DIR $TEST_INDEX > /dev/null
if [ ! -f "$TEST_INDEX" ]; then
    echo -e "${RED}[FAIL] Index file not created.${NC}"
    exit 1
fi
echo -e "${GREEN}[OK] Index built at $TEST_INDEX${NC}"

# 4. Проверка модулей
echo "[4] Verifying Logic..."

# --- Тест 4.1: Стемминг (Stemming) ---
# Ищем слово "apple". Оно должно найти doc1 (там "Apples") и doc2 (там "Apple").
# Это доказывает, что стеммер работает (apples -> apple/appl).
echo "apple" | ./search_cli $TEST_INDEX > tests/out_stem.txt 2>&1
if grep -q "doc1.txt" tests/out_stem.txt && grep -q "doc2.txt" tests/out_stem.txt; then
    echo -e "   [Stemming Check] Query 'apple' found 'Apples': ${GREEN}PASS${NC}"
else
    echo -e "   [Stemming Check] Query 'apple' failed: ${RED}FAIL${NC}"
    cat tests/out_stem.txt
fi

# --- Тест 4.2: Токенизация и регистр ---
# Ищем "BANANA" (капсом). Должно найти "Banana".
echo "BANANA" | ./search_cli $TEST_INDEX > tests/out_token.txt 2>&1
if grep -q "doc2.txt" tests/out_token.txt; then
    echo -e "   [Tokenization Check] Case insensitivity: ${GREEN}PASS${NC}"
else
    echo -e "   [Tokenization Check] Case insensitivity: ${RED}FAIL${NC}"
fi

# --- Тест 4.3: Булев поиск (AND) ---
# "apple AND running". Должен быть только doc1.
echo "apple AND running" | ./search_cli $TEST_INDEX > tests/out_bool.txt 2>&1
if grep -q "doc1.txt" tests/out_bool.txt && ! grep -q "doc2.txt" tests/out_bool.txt; then
    echo -e "   [Boolean AND] 'apple AND running': ${GREEN}PASS${NC}"
else
    echo -e "   [Boolean AND] failed: ${RED}FAIL${NC}"
fi

# 5. Закон Ципфа (наличие файла)
if [ -f "${TEST_INDEX}.csv" ]; then
     echo -e "   [Zipf Check] Stats file generated: ${GREEN}PASS${NC}"
else
     echo -e "   [Zipf Check] Stats file missing: ${RED}FAIL${NC}"
fi

# Очистка
rm -f tests/out_*.txt
echo "=== ALL TESTS COMPLETED ==="