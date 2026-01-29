#!/bin/bash

# Build script for Particle System macOS app

echo "Building Particle System..."

# Compile
g++ -o particles particles.cpp $(sdl2-config --cflags --libs)

if [ $? -eq 0 ]; then
    echo "Compilation successful!"

    # Create app bundle structure if it doesn't exist
    mkdir -p "ParticleSystem.app/Contents/MacOS"
    mkdir -p "ParticleSystem.app/Contents/Resources"

    # Copy executable to app bundle
    cp particles "ParticleSystem.app/Contents/MacOS/ParticleSystem"

    # Generate icon if it doesn't exist
    if [ ! -f "ParticleSystem.app/Contents/Resources/ParticleSystem.icns" ]; then
        echo "Generating app icon..."
        python3 generate_icon.py
    fi

    # Touch to refresh Finder/Dock icon cache
    touch ParticleSystem.app

    echo ""
    echo "App bundle ready: ParticleSystem.app"
    echo ""
    echo "To run:"
    echo "  Double-click ParticleSystem.app in Finder"
    echo "  Or: open ParticleSystem.app"
    echo "  Or: ./particles (command line)"
else
    echo "Build failed!"
    exit 1
fi
