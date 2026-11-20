# Security policy

## Supported versions

| Version | Supported |
|---------|-----------|
| 2.x     | Yes       |

## Scope

Meadows runs untrusted programs in two ways, and both are designed to contain
them:

- **Browser.** Programs run in a Web Worker on a WebAssembly interpreter with no
  access to the page, the network or the host file system.
- **Compiler and `--run`.** Programs have no built-ins for files, processes or
  the network. Resource use is bounded: call depth, string and array length,
  source size (10 MB), nesting depth and import depth are all limited. Import
  paths must be relative, end in `.ms` and avoid `..`.

A compiled program is a normal native executable and runs with the privileges of
the user who starts it.

Reports of crashes, hangs, memory-safety bugs, escapes from these limits and
disagreements between back ends are all welcome.

## Reporting a vulnerability

Do not open a public issue. Report through GitHub's private advisory form:
<https://github.com/nandan-varma/meadows/security/advisories/new>.

We acknowledge reports within 72 hours and aim to fix confirmed issues within
14 days.
