#!/bin/bash
# generates the .expected files by running each test and capturing its output.
# READ THE OUTPUT AFTERWARDS — if a program prints something wrong, thats a bug
# to fix, not something to bake into a test.
BINARY="./tingcc"
GOOD_DIR="testing/codegen/good"

for ting_file in "$GOOD_DIR"/*.ting; do
    base="${ting_file%.ting}"

    "$BINARY" "$ting_file" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "=== $(basename $ting_file): COMPILE FAILED ==="
        "$BINARY" "$ting_file" 2>&1 | head -3
        echo ""
        continue
    fi

    "$base" > "${base}.expected" 2>&1
    echo "=== $(basename $ting_file) ==="
    cat "${base}.expected"
    echo ""
    echo ""

    rm -f "$base"
done
