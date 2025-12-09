#!/usr/bin/env bash
# tests/run_tests.sh
# Run the myshell smoke tests and report PASS/FAIL.
# Requires: ./myshell built in the repository root.
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/.." && pwd)"
TESTDIR="$HERE/expected"
ACTUAL="$HERE/actual"
MS="$ROOT/myshell"

mkdir -p "$TESTDIR" "$ACTUAL"

# helper: run a command stream through myshell and capture stdout
run_shell() {
  local script="$1"
  local out="$2"
  printf "%s\nexit\n" "$script" | "$MS" > "$out" 2>&1 || true
  # remove trailing blank lines
  sed -i.bak -E ':a; /^\s*$/ { N; $!ba; s/(\n\s*)+$//' "$out" 2>/dev/null || true
}

# Test 01: echo simple
cat > "$TESTDIR/01_echo.txt" <<'EOF'
hello
EOF
run_shell 'echo hello' "$ACTUAL/01_echo.txt"

# Test 02: quoted argument
cat > "$TESTDIR/02_quoted.txt" <<'EOF'
quoted arg test
EOF
run_shell 'echo "quoted arg test"' "$ACTUAL/02_quoted.txt"

# Test 03: output redirection then read
cat > "$TESTDIR/03_redir.txt" <<'EOF'
saved
EOF
run_shell 'echo "saved" > out.txt; cat out.txt' "$ACTUAL/03_redir.txt"

# Test 04: printf and sort via pipe
cat > "$TESTDIR/04_sort_pipe.txt" <<'EOF'
a
b
c
EOF
# write input via printf inside shell and pipe to sort
run_shell "printf 'b\nc\na\n' > foo.txt; cat foo.txt | sort" "$ACTUAL/04_sort_pipe.txt"

# Test 05: input redirection
cat > "$TESTDIR/05_sort_stdin.txt" <<'EOF'
a
b
c
EOF
# create file then run sort < foo.txt
run_shell "printf 'b\nc\na\n' > foo2.txt; sort < foo2.txt" "$ACTUAL/05_sort_stdin.txt"

# Test 06: background start prints [bg] line
# We check that "[bg]" text appears (exact PID varies)
cat > "$TESTDIR/06_bg.txt" <<'EOF'
[bg]
EOF
run_shell 'sleep 1 &' "$ACTUAL/06_bg.txt"
# reduce actual to lines containing [bg] to ease comparison
grep '\[bg\]' "$ACTUAL/06_bg.txt" > "$ACTUAL/06_bg_grep.txt" || true
# use grep result for comparison
mv "$ACTUAL/06_bg_grep.txt" "$ACTUAL/06_bg.txt"

# Test 07: overwrite redirection behavior
cat > "$TESTDIR/07_overwrite.txt" <<'EOF'
y
EOF
run_shell 'echo x > f.txt; echo y > f.txt; cat f.txt' "$ACTUAL/07_overwrite.txt"

# Test 08: malformed pipe (should print syntax error message)
cat > "$TESTDIR/08_malformed_pipe.txt" <<'EOF'
syntax error: misplaced pipe
EOF
run_shell '| ls' "$ACTUAL/08_malformed_pipe.txt"

# Test 09: missing redirection filename (syntax error)
cat > "$TESTDIR/09_missing_redir.txt" <<'EOF'
syntax error: expected filename after '>'
EOF
run_shell 'ls >' "$ACTUAL/09_missing_redir.txt"

# Test 10: builtin cd affects next command (semicolon sequencing)
# We expect root "/" on most unixes; if on your environment /tmp is used adjust expected
cat > "$TESTDIR/10_cd_pwd.txt" <<'EOF'
/
EOF
run_shell 'cd /; pwd' "$ACTUAL/10_cd_pwd.txt"

# Test 11: cat+grep into file then cat that file
cat > "$TESTDIR/11_pipe_redir.txt" <<'EOF'
a
EOF
run_shell "printf 'a\nb\n' > foo3.txt; cat foo3.txt | grep a > found.txt; cat found.txt" "$ACTUAL/11_pipe_redir.txt"

# Test 12: semicolon sequencing multiple commands
cat > "$TESTDIR/12_semicolon.txt" <<'EOF'
hello
there
EOF
run_shell 'echo hello; echo there' "$ACTUAL/12_semicolon.txt"

# Now compare expected vs actual for each test
echo "Running comparisons..."
pass=0; fail=0
for t in "$TESTDIR"/*.txt; do
  name="$(basename "$t")"
  act="$ACTUAL/$name"
  if [ ! -f "$act" ]; then
    echo "MISSING actual for $name"
    fail=$((fail+1))
    continue
  fi
  if diff -u "$t" "$act" >/dev/null 2>&1; then
    echo "[PASS] $name"
    pass=$((pass+1))
  else
    echo "[FAIL] $name"
    echo "---- expected ($t) ----"
    sed -n '1,80p' "$t"
    echo "---- actual ($act) ----"
    sed -n '1,80p' "$act"
    fail=$((fail+1))
  fi
done

echo
echo "Summary: $pass passed, $fail failed (total $((pass+fail)))"

# Exit nonzero on failures for CI
if [ "$fail" -ne 0 ]; then
  exit 2
fi
exit 0
