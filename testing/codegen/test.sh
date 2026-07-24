#!/bin/bash
BINARY="./tingcc"
GOOD_DIR="testing/codegen/good"
BAD_DIR="testing/codegen/bad"
PASS=0
FAIL=0

# remove old output files
rm -f "$GOOD_DIR"/*.out "$BAD_DIR"/*.out

echo "===== CODEGEN TESTS ====="

# --- Good tests ---
echo ""
echo "--- Good cases ---"
for ting_file in "$GOOD_DIR"/*.ting; do
    base="${ting_file%.ting}"
    expected_file="${base}.expected"

    if [ ! -f "$expected_file" ]; then
        continue
    fi

    # 1. compile the .ting file (this also runs gcc on the generated c)
    compile_output=$("$BINARY" "$ting_file" 2>&1)
    if [ $? -ne 0 ]; then
        echo ""
        echo "  [FAIL] $ting_file — compiling failed"
        echo "    --- compiler output ---"
        echo "$compile_output"
        ((FAIL++))
        continue
    fi

    # 2. run the program that was made ("$base" because tingcc strips the .ting)
    actual=$("$base" 2>&1)
    expected=$(cat "$expected_file")

    # 3. compare its output to what we expected
    if [ "$actual" = "$expected" ]; then
        ((PASS++))
    else
        out_file="${base}.out"
        echo "$actual" > "$out_file"
        echo ""
        echo "  [FAIL] $ting_file — wrong output"
        echo "    --- expected ---"
        echo "$expected"
        echo "    --- actual ---"
        echo "$actual"
        echo "    saved output: $out_file"
        ((FAIL++))
    fi

    rm -f "$base" # delete the compiled program
done

# --- Bad tests ---
echo ""
echo "--- Bad cases (should error) ---"
for ting_file in "$BAD_DIR"/*.ting; do
    actual=$("$BINARY" "$ting_file" 2>&1)
    exit_code=$?
    if [ $exit_code -ne 0 ]; then
        ((PASS++))
    else
        echo ""
        echo "  [FAIL] $ting_file — should have errored but didn't"
        echo "    --- actual output ---"
        echo "$actual"
        ((FAIL++))

        rm -f "${ting_file%.ting}" # clean up the program it shouldnt have made
    fi
done

# --- Summary ---
echo ""
echo "===== RESULTS: $PASS passed, $FAIL failed ====="
if [ $FAIL -ne 0 ]; then
    exit 1
fi