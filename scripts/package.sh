#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="${1:-1.0}"
ARCHIVE_BASENAME="TitanX${VERSION}-linux-x86_64"
BUILD_DIR="$ROOT_DIR/dist/$ARCHIVE_BASENAME"
ARCHIVE_PATH="$ROOT_DIR/dist/${ARCHIVE_BASENAME}.tar.gz"

mkdir -p "$ROOT_DIR/dist"
rm -rf "$BUILD_DIR"
mkdir -p "$BUILD_DIR/bin" "$BUILD_DIR/scripts" "$BUILD_DIR/src"

make -C "$ROOT_DIR" clean all

cp "$ROOT_DIR/titanx" "$BUILD_DIR/bin/"
cp "$ROOT_DIR/Makefile" "$BUILD_DIR/"
cp "$ROOT_DIR/src"/*.c "$BUILD_DIR/src/"
cp "$ROOT_DIR/src"/*.h "$BUILD_DIR/src/"
cp "$ROOT_DIR/scripts/bench.sh" "$BUILD_DIR/scripts/"

cat > "$BUILD_DIR/README.txt" <<README
TitanX ${VERSION} Linux Package

Run:
  ./bin/titanx /bin/echo hello

Rebuild from source:
  make
README

tar -C "$ROOT_DIR/dist" -czf "$ARCHIVE_PATH" "$ARCHIVE_BASENAME"

echo "Created package: $ARCHIVE_PATH"
