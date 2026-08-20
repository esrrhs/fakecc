#!/usr/bin/env python3
"""Translate the fakecc compiler source into fakecc-self-compilable source.

Pipeline per file:
  1. gcc -E -P with minimal fake system headers (expands fakecc's own macros
     and includes; resolves <stdint.h> etc. to tiny stubs).
  2. Post-process: strip __attribute__, rewrite __builtin_ctzll.
  3. Rewrite anonymous struct/union forms into tagged forms:
       - `typedef struct { ... } Name;` ->
         `struct Name { ... }; typedef struct Name Name;`
       - anonymous struct/union members inside unions get hoisted to unique
         top-level `struct/union <tag> { ... };` definitions and referenced
         as `struct/union <tag> field;`.
     fakecc accepts both the original and the rewritten spelling now, so this
     step is no longer required for the bootstrap to work.  It is kept because
     dropping it rewrites every v0/*.c, and that is worth doing on its own
     rather than as a side effect.
  4. Prepend `package main;`, a "do not edit" marker, and a small
     __builtin_ctzll helper.

v0/*.c is generated. Edit src/ and re-run this script; do not hand-edit v0/.
"""
import os
import re
import subprocess
import sys

SRC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "src")
INC = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "include")
FAKE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fakeinc")
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)))

CTZ_HELPER = """
static int __fakecc_ctzll(unsigned long _v){int c;for(c=0;!(_v&1);c++)_v>>=1;return c;}
static void __fakecc_va_copy(void *dst, void *src){
    char *d = (char*)dst; char *s = (char*)src;
    for(int i = 0; i < 24; i++) d[i] = s[i];
}
"""



def preprocess(path):
    """Run gcc -E -P with fake system headers; return preprocessed text."""
    cmd = ["gcc", "-E", "-P", "-nostdinc", "-DFAKECC_SELFHOST",
           "-I", FAKE, "-I", INC, path]
    r = subprocess.run(cmd, capture_output=True, text=True)
    if r.returncode != 0:
        sys.stderr.write(f"PREPROCESS FAIL {path}:\n{r.stderr}\n")
        sys.exit(1)
    return r.stdout


def strip_va_copy_extern(text):
    """Remove `extern ... __fakecc_va_copy(...)` declarations so static
    definitions in the preamble don't conflict."""
    return re.sub(r'extern void __fakecc_va_copy\([^)]*\)\s*;', '', text)


def strip_attributes(text):
    # `inline` is NOT stripped: fakecc's parser accepts it as a no-op
    # specifier, and a blanket regex also hits the keyword table's own
    # `"inline"` string literal in lexer.c — which leaves the bootstrap
    # compiler unable to recognize the keyword at all.
    # __attribute__((anything nested-parens)) -> empty
    out = []
    i = 0
    while i < len(text):
        if text[i:i+13] == "__attribute__":
            # scan past __attribute__, expect ((...))
            j = i + 13
            while j < len(text) and text[j] in " \t":
                j += 1
            if j < len(text) and text[j] == "(":
                # match ((...))
                if j+1 < len(text) and text[j+1] == "(":
                    depth = 2
                    j += 2
                    while j < len(text) and depth > 0:
                        if text[j] == "(":
                            depth += 1
                        elif text[j] == ")":
                            depth -= 1
                        j += 1
                    i = j
                    continue
        out.append(text[i])
        i += 1
    return "".join(out)


def rewrite_builtin(text):
    # __builtin_ctzll(x) -> __fakecc_ctzll(x)
    return text.replace("__builtin_ctzll(", "__fakecc_ctzll(")


# Adjacent string literals are deliberately NOT merged here.  fakecc's parser
# concatenates them itself, and merging them textually is unsound: `\x` eats as
# many hex digits as it can, so `"\x7f" "ELF"` (4 bytes) would become
# `"\x7fELF"`, where `\x7fE` is one out-of-range escape — 3 bytes starting with
# 0xFE.  That silently corrupted the ELF magic in emit.c and link.c.


# ---------------------------------------------------------------------------
# Tokenizer for struct rewriting
# ---------------------------------------------------------------------------
def tokenize(text):
    """Simple C token stream: strings, chars, and single punctuation tokens."""
    toks = []
    i = 0
    n = len(text)
    while i < n:
        c = text[i]
        if c == '"':
            j = i + 1
            while j < n:
                if text[j] == "\\":
                    j += 2
                    continue
                if text[j] == '"':
                    j += 1
                    break
                j += 1
            toks.append(text[i:j])
            i = j
        elif c == "'":
            j = i + 1
            while j < n and text[j] != "'":
                if text[j] == "\\":
                    j += 1
                j += 1
            if j < n:
                j += 1
            toks.append(text[i:j])
            i = j
        elif c == "/" and i+1 < n and text[i+1] == "/":
            j = text.find("\n", i)
            if j < 0:
                j = n
            toks.append(text[i:j])
            i = j
        elif c == "/" and i+1 < n and text[i+1] == "*":
            j = text.find("*/", i+2)
            if j < 0:
                j = n
            else:
                j += 2
            toks.append(text[i:j])
            i = j
        elif c in "{}();,*[]":
            toks.append(c)
            i += 1
        elif c in " \t\n\r":
            # collapse whitespace to single space (preserve newlines minimally)
            toks.append(c)
            i += 1
        else:
            j = i
            while j < n and text[j] not in "{}();,*[] \t\n\r'\"":
                if text[j] == "/" and j+1 < n and text[j+1] in "/*":
                    break
                j += 1
            toks.append(text[i:j])
            i = j
    return toks


def hoist_body_members(body, counter=None):
    return body


def _hoist_body_members_once(body, counter):
    out = []
    hoisted = []
    i = 0
    n = len(body)
    while i < n:
        # look for `struct {` or `union {` at this position
        m = re.match(r'(struct|union)\s*\{', body[i:])
        if m:
            kind = m.group(1)
            brace_start = i + m.end() - 1  # index of '{'
            # find matching '}'
            depth = 0
            j = brace_start
            while j < n:
                if body[j] == '{':
                    depth += 1
                elif body[j] == '}':
                    depth -= 1
                    if depth == 0:
                        break
                j += 1
            if depth != 0 or j >= n:
                out.append(body[i])
                i += 1
                continue
            # body[brace_start+1:j] is inside the braces
            inner = body[brace_start + 1:j]
            # find the field name after '}'
            k = j + 1
            while k < n and body[k] in ' \t\n\r':
                k += 1
            nm_match = re.match(r'([A-Za-z_]\w*)\s*;', body[k:])
            if nm_match:
                field_name = nm_match.group(1)
                counter[0] += 1
                tag = f"__anon_{field_name}_{counter[0]}"
                # recurse into inner first
                inner_processed = hoist_body_members(inner, counter)
                hoisted.append(f"{kind} {tag} {{{inner_processed}}};")
                out.append(f"{kind} {tag} {field_name};")
                i = k + nm_match.end()
                continue
        out.append(body[i])
        i += 1
    if hoisted:
        return "\n".join(hoisted) + "\n" + "".join(out)
    return "".join(out)


def split_multi_decl(body):
    """Split multi-declarator fields in a struct body: `Type a, b;` -> `Type a;
    Type b;`.  Only touches lines whose first token is a type-like identifier
    and which contain a ',' before ';'."""
    # fakecc now supports bitfields; keep `: N` intact so struct layout
    # matches the original (critical for sizeof(Type) which is used by
    # malloc calls throughout the compiler).
    lines = body.split("\n")
    out = []
    for ln in lines:
        s = ln.strip()
        if "," in s and s.endswith(";") and not s.startswith("/*"):
            m = re.match(r'^([A-Za-z_][A-Za-z0-9_ *]*)\s+(.+);$', s)
            if m:
                typ, decls = m.group(1), m.group(2)
                parts = [p.strip() for p in decls.split(",")]
                if parts and all(re.match(r'^[A-Za-z_]\w*$', p) for p in parts):
                    for p in parts:
                        out.append(f"    {typ} {p};")
                    continue
        out.append(ln)
    return "\n".join(out)


def rewrite_structs(text):
    """Rewrite anonymous struct/union forms to tagged forms fakecc accepts.

    fakecc requires a named struct/union definition before any use and rejects
    inline `struct Tag { } member;`.  Anonymous members must be hoisted to a
    standalone definition just before the enclosing struct/union that contains
    them.  Two-pass: pass 1 collects (parent_kw_idx -> hoisted defns), pass 2
    emits them when reaching the parent keyword."""
    # Global bitfield strip (operates on struct bodies within split_multi_decl).
    toks = tokenize(text)
    n = len(toks)
    anon_counter = [0]
    def new_tag(hint):
        anon_counter[0] += 1
        return f"__anon_{hint}_{anon_counter[0]}"
    WS = {" ", "\t", "\n", "\r"}
    def skip_ws(i):
        while i < n and toks[i] in WS:
            i += 1
        return i

    # Precompute matching brace for every '{'/'}'.  Strings skipped.
    match = [-1] * n
    stack = []
    i = 0
    while i < n:
        t = toks[i]
        if t == "{":
            stack.append(i)
        elif t == "}":
            if stack:
                j = stack.pop()
                match[i] = j
                match[j] = i
        i += 1

    def count_braces(lo, hi):
        d = 0
        k = lo
        while k < hi:
            if toks[k] == "{":
                d += 1
            elif toks[k] == "}":
                d -= 1
            k += 1
        return d

    # Pass 1: find anonymous members and their parent keyword index.
    # parent = nearest enclosing struct/union keyword index (via def_stack).
    hoist = {}  # parent_kw_idx -> [defn, ...]
    def_stack = []
    bd = 0
    i = 0
    while i < n:
        t = toks[i]
        if t == "{":
            bd += 1
        elif t == "}":
            bd -= 1
            while def_stack and def_stack[-1][2] >= bd:
                def_stack.pop()
        if t in ("struct", "union"):
            a = skip_ws(i + 1)
            if a < n and re.match(r'^[A-Za-z_]\w*$', toks[a]):
                # named struct/union def: push and DESCEND into body (don't skip)
                b = skip_ws(a + 1)
                if b < n and toks[b] == "{":
                    def_stack.append([t, i, bd])
                    bd += 1
                    i += 1
                    continue
            elif a < n and toks[a] == "{":
                # Could be a typedef struct { } Name; (top-level type def) or an
                # anonymous member.  If preceded by typedef, skip the whole
                # typedef (it has no parent and must not be hoisted).
                is_typedef = False
                p = i - 1
                while p >= 0 and toks[p] in WS:
                    p -= 1
                if p >= 0 and toks[p] == "typedef":
                    is_typedef = True
                if is_typedef:
                    be = match[a]
                    if be >= 0:
                        j = skip_ws(be + 1)
                        while j < n and toks[j] != ";":
                            j += 1
                        i = j + 1
                        continue
                # anonymous member struct { } name ;
                be = match[a]
                if be >= 0:
                    fn = skip_ws(be + 1)
                    if fn < n and re.match(r'^[A-Za-z_]\w*$', toks[fn]):
                        k2 = skip_ws(fn + 1)
                        if k2 < n and toks[k2] == ";":
                            body = split_multi_decl("".join(toks[a + 1:be]))
                            tag = new_tag(toks[fn])
                            defn = f"{t} {tag} {{{body}}};"
                            parent_idx = def_stack[-1][1] if def_stack else None
                            if parent_idx is not None:
                                hoist.setdefault(parent_idx, []).append(defn)
                            i = fn + 1
                            continue
        i += 1

    # Pass 2: emit tokens, converting typedef structs and inserting hoisted defs.
    out = []
    def_stack = []
    bd = 0
    i = 0
    while i < n:
        t = toks[i]
        if t == "{":
            bd += 1
        elif t == "}":
            bd -= 1
            while def_stack and def_stack[-1][2] >= bd:
                def_stack.pop()

        # Pattern 1: typedef struct/enum { ... } Name ;
        if t == "typedef":
            a = skip_ws(i + 1)
            if a < n and toks[a] in ("struct", "enum"):
                kind = toks[a]
                b = skip_ws(a + 1)
                if b < n and toks[b] == "{":
                    be = match[b]
                    if be >= 0:
                        j = skip_ws(be + 1)
                        names = []
                        while j < n and toks[j] != ";":
                            if toks[j].strip() and toks[j] not in ",":
                                names.append(toks[j])
                            j += 1
                        body = hoist_body_members(split_multi_decl("".join(toks[b + 1:be])), anon_counter)
                        tag = names[0] if names else new_tag("td")
                        if kind == "struct":
                            out.append(f"struct {tag} {{{body}}};")
                            out.append(f"typedef struct {tag} {' '.join(names)};")
                        else:
                            out.append(f"enum {tag} {{{body}}};")
                            out.append(f"typedef enum {tag} {' '.join(names)};")
                        bd += count_braces(i, j + 1)
                        i = j + 1
                        continue
                # typedef struct Name { body } Names — named struct
                elif b < n and re.match(r'^[A-Za-z_]\w*$', toks[b]):
                    struct_tag = toks[b]
                    c = skip_ws(b + 1)
                    if c < n and toks[c] == "{":
                        be = match[c]
                        if be >= 0:
                            j = skip_ws(be + 1)
                            names = []
                            while j < n and toks[j] != ";":
                                if toks[j].strip() and toks[j] not in ",":
                                    names.append(toks[j])
                                j += 1
                            body = hoist_body_members(split_multi_decl("".join(toks[c + 1:be])), anon_counter)
                            if kind == "struct":
                                out.append(f"struct {struct_tag} {{{body}}};")
                            else:
                                out.append(f"enum {struct_tag} {{{body}}};")
                            if names:
                                if kind == "struct":
                                    out.append(f"typedef struct {struct_tag} {' '.join(names)};")
                                else:
                                    out.append(f"typedef enum {struct_tag} {' '.join(names)};")
                            bd += count_braces(i, j + 1)
                            i = j + 1
                            continue

        # Named struct/union definition opening: struct Tag { ... };
        if t in ("struct", "union"):
            # emit hoisted member defs right before this parent keyword
            if i in hoist:
                for d in hoist[i]:
                    out.append(d)
            kind = t
            a = skip_ws(i + 1)
            if a < n and re.match(r'^[A-Za-z_]\w*$', toks[a]):
                b = skip_ws(a + 1)
                if b < n and toks[b] == "{":
                    be = match[b]
                    if be >= 0:
                        j = skip_ws(be + 1)
                        trailer = ""
                        while j < n and toks[j] != ";":
                            trailer += toks[j]
                            j += 1
                        body = hoist_body_members(split_multi_decl("".join(toks[b + 1:be])), anon_counter)
                        out.append(f"{kind} {toks[a]} {{{body}}};")
                        if trailer.strip():
                            out.append(f"typedef {kind} {toks[a]} {trailer.strip()};")
                        def_stack.append([toks[a], i, bd])
                        bd += count_braces(i, j + 1)
                        i = j + 1
                        continue
            # Anonymous member (only reached if not consumed above)
            if a < n and toks[a] == "{":
                be = match[a]
                if be >= 0:
                    fn = skip_ws(be + 1)
                    if fn < n and re.match(r'^[A-Za-z_]\w*$', toks[fn]):
                        k2 = skip_ws(fn + 1)
                        if k2 < n and toks[k2] == ";":
                            i = fn + 1
                            continue

        out.append(t)
        i += 1

    return "".join(out)



def strip_ptr_qualifiers(text):
    """Strip const/volatile/restrict that appear after a `*` in declarators.
    fakecc rejects `char * const *p` (qualifier after star) though it accepts
    the leading `const char *p`.  These inner qualifiers are no-ops for fakecc,
    so drop them.  e.g. `Type * const *p` -> `Type **p`."""
    prev = None
    while prev != text:
        prev = text
        text = re.sub(r'(\*)\s*(const|volatile|restrict)\b', r'\1', text)
    return text


def split_global_multidecl(text):
    """Split multi-declarator statements outside of function parameter lists.
    `Type a, b;` -> `Type a; Type b;`.  Avoids lines containing '(' before ';'
    (those are function declarations/calls, which must keep their commas)."""
    lines = text.split("\n")
    out = []
    for ln in lines:
        s = ln.strip()
        if "," in s and s.endswith(";") and "(" not in s.split(";")[0]:
            m = re.match(r'^(\s*)([A-Za-z_][A-Za-z0-9_ *]*)\s+(.+);$', s)
            if m:
                indent, typ, decls = m.group(1), m.group(2), m.group(3)
                parts = [p.strip() for p in decls.split(",")]
                if parts and all(re.match(r'^[A-Za-z_]\w*$', p) for p in parts):
                    for p in parts:
                        out.append(f"{indent}{typ} {p};")
                    continue
        out.append(ln)
    return "\n".join(out)


def hoist_local_structs(text):
    """Hoist local typedef structs to file scope.
    fakecc rejects struct definitions inside functions.  rewrite_structs converts
    a local `typedef struct {...} Name;` into `struct Name {...}; typedef struct
    Name Name;` in place; here we detect those indented `struct Name {...};`
    immediately followed by `typedef struct Name Name;` and move both to file
    scope.  The two known cases: SymInfo (link.c) and DFSFrame (mem2reg.c)."""
    lines = text.split("\n")
    hoisted = []
    kept = []
    skip = False
    for idx, ln in enumerate(lines):
        if skip:
            skip = False
            continue
        stripped = ln.lstrip()
        indent = ln[:len(ln) - len(stripped)]
        if indent and re.match(r'struct\s+[A-Za-z_]\w*\s*\{.*\};', stripped):
            # Check for a trailing "typedef struct Name Name;" on the same line
            ss = re.search(r'\};typedef struct\s+(\w+)\s+(\w+)\s*;$', stripped)
            if ss and ss.group(1) == ss.group(2) and ss.group(1) in stripped:
                hoisted.append(re.sub(r'\};.*', '};', stripped))
                hoisted.append(f"typedef struct {ss.group(1)} {ss.group(1)};")
                skip = False  # already consumed
                continue
            # Or on the next line
            nxt_idx = idx + 1
            while nxt_idx < len(lines) and not lines[nxt_idx].strip():
                nxt_idx += 1
            nxt = lines[nxt_idx].strip() if nxt_idx < len(lines) else ""
            m = re.match(r'typedef struct\s+(\w+)\s+(\w+)\s*;', nxt)
            if m and m.group(1) == m.group(2) and m.group(1) in stripped:
                hoisted.append(stripped)
                hoisted.append(nxt)
                skip = True
                continue
        kept.append(ln)
    if hoisted:
        # Insert hoisted defs after all initial declarations but
        # before the first function body.
        insert_at = len(kept)
        for idx, ln in enumerate(kept):
            s = ln.strip()
            if s.startswith("package "):
                continue
            # A function definition: `type name(...)` with `{` on this
            # or a following line (not a declaration ending with `;`).
            if re.match(r'^\w[\w\s*]+\s+\w+\s*\(', s) and not s.endswith(';') \
               and not s.startswith("typedef ") \
               and not s.startswith("struct ") and not s.startswith("union ") \
               and not s.startswith("enum ") and not s.startswith("extern ") \
               and not s.startswith("const "):
                # If this line has `{` or the next non-blank line is `{`,
                # this is a function body start.
                if '{' in s:
                    insert_at = idx
                    break
                # Check next non-blank line
                nxt = idx + 1
                while nxt < len(kept) and not kept[nxt].strip():
                    nxt += 1
                if nxt < len(kept) and kept[nxt].strip() == '{':
                    insert_at = idx
                    break
        result = kept[:insert_at] + hoisted + kept[insert_at:]
        return "\n".join(result)
    return "\n".join(kept)


def cleanup_unused_definitions(body):
    """Drop all extern declarations.  v0 files are compiled as a unit (all
    together, not `-c` per file), so fakecc's package-main semantics expose
    sibling files' symbols unqualified — cross-file references like
    get_ir_structs resolve without extern.  Runtime symbols are also
    available via `import runtime;` with qualified calls.  Nothing is left
    that needs an extern."""
    result = []
    for ln in body.split("\n"):
        s = ln.strip()
        # A real extern declaration starts with `extern` followed by a type
        # keyword or qualifier (not an identifier like `extern_sym`).
        if re.match(r'^extern\s+(?:int|void|char|long|short|float|double|'
                    r'unsigned|signed|size_t|ptrdiff_t|ssize_t|intptr_t|'
                    r'uintptr_t|const|struct|enum|FILE)\b', s):
            continue
        result.append(ln)
    return "\n".join(result)



# Functions provided by the builtin runtime/ package.  Calls to these are
# rewritten to qualified `runtime.<name>` form so the file can drop its
# per-file `extern` declarations and rely on `import runtime;` instead.
RUNTIME_FUNCS = frozenset({
    "abort", "atoi", "atol", "calloc", "chmod", "exit", "fclose", "fflush",
    "fileno", "fopen", "fprintf", "fputc", "fputs", "fread", "free", "fseek",
    "ftell", "fwrite", "getenv", "isalnum", "isalpha", "isdigit", "isspace",
    "isxdigit", "malloc", "memcmp", "memcpy", "memmove", "memset", "perror",
    "printf", "putchar", "puts", "qsort", "realloc", "snprintf", "sprintf",
    "strcmp", "strchr", "strcpy", "strdup", "strerror", "strlen", "strncmp",
    "strncpy", "strrchr", "strstr", "strtod", "strtof", "strtold", "strtol",
    "strtoul", "strtoull", "vfprintf", "vsnprintf", "vsprintf",
})


def qualify_runtime_calls(body):
    """Rewrite calls to runtime functions as runtime.<name>(...).  This lets
    the file use `import runtime;` instead of per-file extern declarations."""
    # Simple token-based rewrite: scan for `identifier(` where identifier is a
    # runtime function, and prefix it with `runtime.`.  Avoid rewriting inside
    # string literals and comments.
    result = []
    i = 0
    n = len(body)
    while i < n:
        c = body[i]
        # Skip string literals
        if c == '"':
            j = i + 1
            while j < n:
                if body[j] == "\\":
                    j += 2
                    continue
                if body[j] == '"':
                    j += 1
                    break
                j += 1
            result.append(body[i:j])
            i = j
            continue
        # Skip char literals
        if c == "'":
            j = i + 1
            while j < n and body[j] != "'":
                if body[j] == "\\":
                    j += 1
                j += 1
            if j < n:
                j += 1
            result.append(body[i:j])
            i = j
            continue
        # Skip line comments
        if c == "/" and i + 1 < n and body[i + 1] == "/":
            j = body.find("\n", i)
            if j < 0:
                j = n
            result.append(body[i:j])
            i = j
            continue
        # Skip block comments
        if c == "/" and i + 1 < n and body[i + 1] == "*":
            j = body.find("*/", i + 2)
            if j < 0:
                j = n
            else:
                j += 2
            result.append(body[i:j])
            i = j
            continue
        # Check for identifier followed by '('
        if c.isalpha() or c == "_":
            j = i
            while j < n and (body[j].isalnum() or body[j] == "_"):
                j += 1
            word = body[i:j]
            # Skip if already qualified (preceded by 'runtime.')
            k = i - 1
            while k >= 0 and body[k] in " \t":
                k -= 1
            if k >= 6 and body[k - 6:k + 1] == "runtime":
                result.append(word)
                i = j
                continue
            # Check if we're inside an extern declaration — don't rewrite those.
            line_start = body.rfind("\n", 0, i) + 1
            line_so_far = body[line_start:i].lstrip()
            is_extern_decl = line_so_far.startswith("extern")
            # Check if followed by '(' (possibly with whitespace)
            m = j
            while m < n and body[m] in " \t":
                m += 1
            if word in RUNTIME_FUNCS and m < n and body[m] == "(" and not is_extern_decl:
                result.append("runtime.")
                result.append(word)
                i = j
                continue
            # Rewrite references to runtime globals (stderr, stdin, stdout).
            if word in RUNTIME_GLOBALS and m < n and body[m] != ";" and not is_extern_decl:
                result.append("runtime.")
                result.append(word)
                i = j
                continue
            result.append(word)
            i = j
            continue
        result.append(c)
        i += 1
    return "".join(result)


# Global variables provided by the builtin runtime/ package.
RUNTIME_GLOBALS = frozenset({"stdin", "stdout", "stderr"})


def strip_runtime_externs(body):
    """Remove extern declarations for symbols provided by the runtime package;
    those are now accessed via `import runtime;` and qualified calls."""
    lines = body.split("\n")
    result = []
    for ln in lines:
        s = ln.strip()
        # extern functions: extern int fprintf(FILE *f, ...);
        # The function name is the identifier right before the opening '(' of
        # the parameter list — find the last '(' that isn't followed by ')'
        # (i.e. the outer parameter list, not a function-pointer parameter).
        if s.startswith("extern"):
            # Find the matching '(' for the function's parameter list: it's the
            # first '(' that is directly preceded by the function name (no '*'
            # between name and '(' means it's not a function pointer).
            m = re.search(r'\b([A-Za-z_]\w*)\s*\((?!\s*\*)', s)
            if m and m.group(1) in RUNTIME_FUNCS:
                continue
        # extern global variables: extern FILE *stderr;
        m = re.match(r'^extern\s+.*\b([A-Za-z_]\w*)\s*;', s)
        if m and m.group(1) in RUNTIME_GLOBALS:
            continue
        result.append(ln)
    return "\n".join(result)


def translate_file(src_path, out_path, strip_types=False):
    text = preprocess(src_path)
    text = strip_attributes(text)
    text = strip_va_copy_extern(text)
    text = rewrite_builtin(text)
    body = rewrite_structs(text)
    body = strip_ptr_qualifiers(body)
    body = split_global_multidecl(body)
    body = hoist_local_structs(body)
    # Join multiline function prototypes split before semicolon (e.g. die_at(...)\n    ;)
    body = re.sub(r'(\))\s*\n\s*(;)', r'\1\2', body)
    body = cleanup_unused_definitions(body)
    body = qualify_runtime_calls(body)
    if strip_types:
        body = strip_type_definitions(body)
    preamble = ('package main;\n'
                'import runtime;\n'
                '/* Generated by v0/translate.py from src/ — do not edit. */\n'
                + CTZ_HELPER + '\n')
    with open(out_path, "w") as f:
        f.write(preamble)
        f.write(body)


def strip_type_definitions(body):
    """Keep all type definitions in v0 translated files so each file remains
    self-contained for parsing, while the package system handles pre-tokenization
    and type sharing dynamically at runtime for non-translated user packages."""
    return body


def main():
    os.makedirs(OUT, exist_ok=True)
    files = sorted(f for f in os.listdir(SRC) if f.endswith(".c"))
    # ir.c owns all ir.h type definitions; every other file strips them.
    # The compiler pre-scans all package files for types, so stripping is
    # safe regardless of compilation order.
    for f in files:
        src = os.path.join(SRC, f)
        dst = os.path.join(OUT, f)
        print(f"translate {f}", flush=True)
        translate_file(src, dst, strip_types=(f != "ir.c"))
    print(f"done: {len(files)} files -> {OUT}")


if __name__ == "__main__":
    main()
