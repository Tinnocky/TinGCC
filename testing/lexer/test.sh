#!/bin/bash

BINARY="./tingcc"
GOOD_DIR="testing/lexer/good"
BAD_DIR="testing/lexer/bad"

PASS=0
FAIL=0

# remove old output files
rm -f "$GOOD_DIR"/*.out "$BAD_DIR"/*.out

echo "===== LEXER TESTS ====="

# --- Good tests ---
echo ""
echo "--- Good cases ---"

for ting_file in "$GOOD_DIR"/*.ting; do
    base="${ting_file%.ting}"
    expected_file="${base}.expected"

    if [ ! -f "$expected_file" ]; then
        continue
    fi

    actual=$("$BINARY" "$ting_file" 2>&1)
    expected=$(cat "$expected_file")

    if [ "$actual" = "$expected" ]; then
        ((PASS++))
    else
        out_file="${base}.out"
        echo "$actual" > "$out_file"

        echo ""
        echo "  [FAIL] $ting_file"
        echo "    --- expected ---"
        echo "$expected"
        echo "    --- actual ---"
        echo "$actual"
        echo "    saved output: $out_file"

        ((FAIL++))
    fi
done

# --- Bad tests ---
echo ""
echo "--- Bad cases (should error) ---"

for ting_file in "$BAD_DIR"/*.ting; do
    "$BINARY" "$ting_file" > /dev/null 2>&1
    exit_code=$?

    if [ $exit_code -ne 0 ]; then
        ((PASS++))
    else
        echo ""
        echo "  [FAIL] $ting_file — should have errored but didn't"
        ((FAIL++))
    fi
done

# --- Summary ---
echo ""
echo "===== RESULTS: $PASS passed, $FAIL failed ====="

if [ $FAIL -ne 0 ]; then
    exit 1
fi