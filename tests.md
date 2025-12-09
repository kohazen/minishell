# Test suite — Mini Shell (human-readable)

This file lists the automated tests executed by `tests/run_tests.sh`. The tests map to Lab-1 functional requirements (argument parsing, quotes, I/O redirection, single pipe, background, builtin cd/exit) as specified in the assignment.

**Note about prompt and background PID handling**
- The test harness strips the `myshell>` prompt lines from the captured output before comparison, so expected files do not include the prompt.
- For the background test the shell prints a dynamic PID (e.g., `[bg] pid 12345`). The automated test normalizes this by removing the numeric PID and asserting only that the `[bg]` token appears.

## Tests (summary)

1. **Echo simple**
   - Command: `echo hello`
   - Expected: `hello`

2. **Quoted argument**
   - Command: `echo "quoted arg test"`
   - Expected: `quoted arg test`

3. **Output redirection**
   - Command sequence: `echo "saved" > out.txt; cat out.txt`
   - Expected: `saved`

4. **Printf + pipe + sort**
   - Command sequence: `printf 'b\nc\na\n' > foo.txt; cat foo.txt | sort`
   - Expected: sorted lines `a b c`

5. **Input redirection**
   - Command sequence: `printf 'b\nc\na\n' > foo2.txt; sort < foo2.txt`
   - Expected: sorted lines `a b c`

6. **Background execution**
   - Command: `sleep 1 &`
   - Expected: presence of `[bg]` (PID not asserted)

7. **Overwrite redirection**
   - Command sequence: `echo x > f.txt; echo y > f.txt; cat f.txt`
   - Expected: `y`

8. **Malformed pipe**
   - Command: `| ls`
   - Expected: `syntax error: misplaced pipe`

9. **Missing redirection filename**
   - Command: `ls >`
   - Expected: `syntax error: expected filename after '>'`

10. **Builtin cd affects next command**
    - Command sequence: `cd /; pwd`
    - Expected: `/` (root directory printed)

11. **Pipe into file**
    - Command: `printf 'a\nb\n' > foo3.txt; cat foo3.txt | grep a > found.txt; cat found.txt`
    - Expected: `a`

12. **Semicolon sequencing**
    - Command: `echo hello; echo there`
    - Expected:
      ```
      hello
      there
      ```

---

## How to run

1. Ensure `myshell` binary exists at the repo root (build with `make`).
2. Make the script executable:
   ```bash
   chmod +x tests/run_tests.sh
3. ```bash
   tests/run_tests.sh
