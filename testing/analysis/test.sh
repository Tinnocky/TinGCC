#!/bin/bash
BINARY="./tingcc"
GOOD_DIR="testing/analysis/good"
BAD_DIR="testing/analysis/bad"
PASS=0
FAIL=0

echo "===== ANALYSIS TESTS ====="

# --- Good tests ---
echo ""
echo "--- Good cases (should exit 0) ---"
for ting_file in "$GOOD_DIR"/*.ting; do
    "$BINARY" "$ting_file" > /dev/null 2>&1
    if [ $? -eq 0 ]; then
        ((PASS++))
    else
        actual=$("$BINARY" "$ting_file" 2>&1)
        echo ""
        echo "  [FAIL] $ting_file — should have passed but errored"
        echo "    --- actual output ---"
        echo "$actual"
        ((FAIL++))
    fi
done

# --- Bad tests ---
echo ""
echo "--- Bad cases (should error) ---"
for ting_file in "$BAD_DIR"/*.ting; do
    "$BINARY" "$ting_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
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