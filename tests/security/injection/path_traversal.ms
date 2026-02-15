# Security Test: path_traversal.ms
# Purpose: Verify path traversal attacks are blocked
# Expected: Compiler should reject paths with directory traversal

# These should all be rejected:
# import ../../../etc/passwd;
# import ..\\..\\windows\\system32\\config\\sam;
# import /etc/passwd;

# Valid imports should work:
# import math;
# import std.io;

func main() {
    print "Path traversal test";
    return 0;
}
