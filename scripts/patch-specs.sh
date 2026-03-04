#!/bin/sh
# patch-specs.sh — Patch GCC specs to auto-use libsolcompat
#
# Usage: patch-specs.sh <gcc-prefix> <override-dir> <compat-libdir>
#
# This script modifies GCC's specs file so that every compilation and
# link automatically uses libsolcompat's override headers and libraries.
# No -I, -L, or -l flags needed in build systems.
#
# What it does:
#   1. Dumps the current GCC specs to a file (if no custom specs exist)
#   2. Adds -isystem <override-dir> to *cpp_options (header search)
#   3. Adds -isystem <override-dir> to *cc1 (C compiler)
#   4. Adds -isystem <override-dir> to *cc1plus (C++ compiler)
#   5. Adds -L<compat-libdir> -lsolcompat to *lib (linker)
#
# After patching:
#   ./configure && make     # just works — no special flags
#   AC_CHECK_FUNC(fabsf)    # finds it in augmented libm.a
#   AC_CHECK_HEADER(stdint.h)  # finds it in override dir
#
# Part of libsolcompat — https://github.com/firefly128/libsolcompat

set -e

usage() {
    echo "Usage: $0 <gcc-binary-or-prefix> <override-include-dir> <compat-lib-dir>"
    echo ""
    echo "  gcc-binary-or-prefix:  Path to gcc binary or prefix directory"
    echo "                         e.g., /opt/sst/gcc11/bin/gcc  OR  /opt/sst/gcc11"
    echo "  override-include-dir:  Directory with override headers"
    echo "                         e.g., /opt/sst/lib/solcompat/include/override"
    echo "  compat-lib-dir:        Directory with augmented libs and libsolcompat.a"
    echo "                         e.g., /opt/sst/lib/solcompat/lib"
    exit 1
}

[ $# -ge 3 ] || usage

GCC_ARG="$1"
OVERRIDE_INC="$2"
COMPAT_LIB="$3"

# Resolve GCC binary
if [ -x "$GCC_ARG" ]; then
    GCC="$GCC_ARG"
elif [ -d "$GCC_ARG" ]; then
    # Try to find gcc in bin/
    for g in "$GCC_ARG"/bin/*-gcc "$GCC_ARG"/bin/gcc; do
        if [ -x "$g" ]; then
            GCC="$g"
            break
        fi
    done
fi

if [ -z "$GCC" ] || [ ! -x "$GCC" ]; then
    echo "Error: Cannot find gcc binary from '$GCC_ARG'"
    exit 1
fi

echo "GCC binary: $GCC"
echo "Override includes: $OVERRIDE_INC"
echo "Compat libraries: $COMPAT_LIB"

# Find the specs file location
SPECS_DIR=$("$GCC" -print-file-name=specs 2>/dev/null || true)
if [ -z "$SPECS_DIR" ] || [ "$SPECS_DIR" = "specs" ]; then
    # Fallback: use -print-search-dirs
    SPECS_DIR=$("$GCC" -print-search-dirs | grep '^install:' | sed 's/install: //')
    SPECS_DIR="${SPECS_DIR}/specs"
fi

# If specs doesn't exist, create it from the builtin defaults
SPECS_FILE="$SPECS_DIR"
if [ ! -f "$SPECS_FILE" ]; then
    echo "No custom specs file found. Dumping defaults..."
    "$GCC" -dumpspecs > "$SPECS_FILE"
    echo "  Created: $SPECS_FILE"
fi

echo "Specs file: $SPECS_FILE"

# Back up the original
if [ ! -f "${SPECS_FILE}.orig" ]; then
    cp "$SPECS_FILE" "${SPECS_FILE}.orig"
    echo "  Backed up to: ${SPECS_FILE}.orig"
fi

# Check if already patched
if grep -q 'solcompat' "$SPECS_FILE" 2>/dev/null; then
    echo "Specs file already contains solcompat entries. Skipping."
    exit 0
fi

# Apply patches using sed
# The specs file format: section headers like *cpp: or *cc1:
# followed by the spec string, then a blank line.
#
# Strategy:
#   - After *cpp_options: line, add -isystem to the spec string
#   - After *cc1: line, add -isystem to the spec string
#   - After *cc1plus: line, add -isystem to the spec string  
#   - After *lib: line, add -L and -lsolcompat to the spec string
#
# GCC specs syntax: the value is on the line(s) after the *section: header,
# and we prepend/append our additions.

TMPFILE="${SPECS_FILE}.tmp"
cp "$SPECS_FILE" "$TMPFILE"

# Note: GCC specs format has the section name like "*cc1:" on one line,
# and the spec string on the next line(s). We need to add our flags
# to the end of that spec string.

# For include paths, we use -isystem which acts like -I but with system
# header semantics (warnings suppressed, searched after -I dirs).
# We add TWO paths:
#   1. override/   — wrapper headers with #include_next
#   2. (parent)    — solcompat/ internal headers

OVERRIDE_PARENT=$(dirname "$OVERRIDE_INC")

# Patch *cc1: — add -isystem for C compilations
# This is the safest place: *cc1 gets the flags after preprocessing
sed -i "/^\*cc1:/{
n
s|$| -isystem ${OVERRIDE_INC} -isystem ${OVERRIDE_PARENT}|
}" "$TMPFILE"

# Patch *cc1plus: — add -isystem for C++ compilations
sed -i "/^\*cc1plus:/{
n
s|$| -isystem ${OVERRIDE_INC} -isystem ${OVERRIDE_PARENT}|
}" "$TMPFILE"

# Patch *lib: — add -L and -lsolcompat
# The *lib section controls what gets linked. Adding -lsolcompat here
# means every binary automatically gets the compat symbols.
# We also add -L for the augmented library directory.
sed -i "/^\*lib:/{
n
s|$| -L${COMPAT_LIB} -lsolcompat|
}" "$TMPFILE"

# Verify the patches were applied
CHANGES=0
if grep -q "$OVERRIDE_INC" "$TMPFILE"; then
    CHANGES=$((CHANGES + 1))
fi
if grep -q "lsolcompat" "$TMPFILE"; then
    CHANGES=$((CHANGES + 1))
fi

if [ "$CHANGES" -lt 2 ]; then
    echo "WARNING: Specs patching may have been incomplete ($CHANGES/2 sections patched)"
    echo "The specs file format may differ from expected. Manual review needed."
    echo "Temp file preserved at: $TMPFILE"
    exit 1
fi

mv "$TMPFILE" "$SPECS_FILE"
echo ""
echo "Specs file patched successfully!"
echo "  Override headers:  $OVERRIDE_INC"
echo "  Compat libraries:  $COMPAT_LIB"
echo ""
echo "Verify with: $GCC -v -E - < /dev/null 2>&1 | grep isystem"
echo "  and:       $GCC -v -x c /dev/null -o /dev/null 2>&1 | grep solcompat"
