#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# Clean previous builds
rm -rf build debian/tmp
mkdir -p build

# Configure with installation prefix
meson setup build --prefix=/usr

# Build
cd build
ninja

# Install to temporary directory
DESTDIR="$SCRIPT_DIR/debian/tmp" ninja install
cd "$SCRIPT_DIR"

# Get architecture
ARCH=$(dpkg --print-architecture)

# Create DEBIAN directory
mkdir -p debian/tmp/DEBIAN

# Copy control file and substitute version and architecture
sed "s/Architecture: amd64/Architecture: $ARCH/" debian/control > debian/tmp/DEBIAN/control

# Calculate installed size
INSTALLED_SIZE=$(du -s -k debian/tmp | cut -f1)
echo "Installed-Size: $INSTALLED_SIZE" >> debian/tmp/DEBIAN/control

# Set permissions
chmod 755 debian/tmp/DEBIAN
find debian/tmp -type d -exec chmod 755 {} \;
find debian/tmp -type f -exec chmod 644 {} \;
chmod 755 debian/tmp/usr/bin/topwindow || true

# Build the package
PACKAGE_NAME="topwindow_0.1.0_${ARCH}.deb"
dpkg-deb --build --root-owner-group debian/tmp "$PACKAGE_NAME"

echo ""
echo "Package built successfully: $PACKAGE_NAME"
echo ""
echo "To install on this system: sudo dpkg -i $PACKAGE_NAME"
echo "To fix dependencies if needed: sudo apt-get install -f"
