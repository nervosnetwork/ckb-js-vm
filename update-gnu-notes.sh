#!/bin/bash
# Script to add .note.gnu.property section to the final binary

set -e

BINARY=$1
OBJCOPY=${OBJCOPY:-llvm-objcopy-21}
TEMP_NOTE=$(mktemp)

if [ -z "$BINARY" ]; then
    echo "Usage: $0 <binary-file>"
    exit 1
fi

echo "backup old one"
cp $BINARY $BINARY.old
echo "Before updating..."
llvm-readelf-21 -n $BINARY

# Create the note section content
# This represents: RISC-V feature: ZICFILP-unlabeled, ZICFISS
# Format: ELF note with type NT_GNU_PROPERTY_TYPE_0
cat > "$TEMP_NOTE" << 'EOF_BINARY'
04 00 00 00 10 00 00 00 05 00 00 00 47 4e 55 00
00 00 00 c0 04 00 00 00 03 00 00 00 00 00 00 00
EOF_BINARY

# Convert hex to binary
xxd -r -p "$TEMP_NOTE" > "${TEMP_NOTE}.bin"

# Remove existing .note.gnu.property section if it exists
$OBJCOPY --remove-section .note.gnu.property "$BINARY" 2>/dev/null || true

# Add the note section to the binary
$OBJCOPY --add-section .note.gnu.property="${TEMP_NOTE}.bin" \
         --set-section-flags .note.gnu.property=alloc,readonly \
         --set-section-alignment .note.gnu.property=8 \
         "$BINARY"

# Cleanup
rm -f "$TEMP_NOTE" "${TEMP_NOTE}.bin"

echo "Successfully added .note.gnu.property section to $BINARY"
llvm-readelf-21 -n $BINARY