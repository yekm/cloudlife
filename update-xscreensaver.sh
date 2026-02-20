#!/bin/bash
set -ev

REPO_URL="https://github.com/Zygo/xscreensaver.git"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARCHIVE_DIR="$SCRIPT_DIR/arχiv"
XSRC_DIR="$ARCHIVE_DIR/xscreensaver"
TMP_DIR=$(mktemp -d)

trap "rm -rf $TMP_DIR" EXIT

echo "Cloning xscreensaver repository..."
git clone --depth 1 "$REPO_URL" "$TMP_DIR/xscreensaver"

echo "Updating archive..."

#if [ -d "$XSRC_DIR" ]; then
#    rm -rf "$XSRC_DIR"
#fi

mkdir -p "$XSRC_DIR"

cp -r "$TMP_DIR/xscreensaver/hacks" "$XSRC_DIR/"
cp -r "$TMP_DIR/xscreensaver/utils" "$XSRC_DIR/"
cp "$TMP_DIR/xscreensaver/README" "$XSRC_DIR/" 2>/dev/null || true
cp "$TMP_DIR/xscreensaver/README.hacking" "$XSRC_DIR/" 2>/dev/null || true

find "$XSRC_DIR/" -iname '*.man' -delete


echo "Done. Updated hacks in $XSRC_DIR/hacks/"
ls "$XSRC_DIR/hacks/" | wc -l | xargs echo "Total files:"
