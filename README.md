# Particle System

A simple interactive particle system built with C++ and SDL2. Click anywhere in the window to spawn colorful particles that bounce off walls and fade over time.

## Features

- **Click to spawn** - Creates a burst of 25 particles at the mouse position
- **Physics simulation** - Gravity, velocity, and bounce mechanics
- **Wall collision** - Particles bounce off all four edges with energy dampening
- **Fade effect** - Particles gradually become transparent as they age
- **Automatic cleanup** - Dead particles are removed to maintain performance

## Requirements

- C++ compiler (g++ or clang++)
- SDL2 library

## Installation

### macOS

```bash
brew install sdl2
```

### Ubuntu/Debian

```bash
sudo apt-get install libsdl2-dev
```

### Fedora

```bash
sudo dnf install SDL2-devel
```

### Windows

Download SDL2 development libraries from https://libsdl.org and configure your compiler's include/library paths.

## Building

Compile the project using:

```bash
g++ -o particles particles.cpp $(sdl2-config --cflags --libs)
```

Or with clang:

```bash
clang++ -o particles particles.cpp $(sdl2-config --cflags --libs)
```

## Running

```bash
./particles
```

## Controls

| Input | Action |
|-------|--------|
| Left Click | Spawn particles at cursor |
| ESC | Quit |
| Close Window | Quit |

## How It Works

### Code Structure

The program is organized into several sections:

1. **Particle struct** - Holds data for each particle (position, velocity, color, alpha, lifetime)
2. **spawnParticles()** - Creates a burst of particles with random directions and colors
3. **updateParticles()** - Applies physics each frame (gravity, movement, bouncing, fading)
4. **main()** - Initializes SDL, runs the game loop, handles input

### Physics

Each frame, particles are updated with basic Euler integration:

```
velocity.y += gravity * deltaTime    // Apply gravity
position += velocity * deltaTime     // Move particle
```

When a particle hits a wall, its velocity is reversed and reduced by the bounce damping factor (30% energy loss per bounce).

### Rendering

Particles are drawn as 4x4 pixel squares with alpha blending enabled. As a particle ages, its alpha value decreases proportionally, creating a smooth fade-out effect.

## Customization

Edit these constants at the top of `particles.cpp` to change behavior:

| Constant | Default | Description |
|----------|---------|-------------|
| `WINDOW_WIDTH` | 800 | Window width in pixels |
| `WINDOW_HEIGHT` | 600 | Window height in pixels |
| `GRAVITY` | 500.0 | Downward acceleration (pixels/sec²) |
| `BOUNCE_DAMPING` | 0.7 | Energy retained after bounce (0.0-1.0) |
| `PARTICLE_LIFETIME` | 3.0 | Seconds before particle fades completely |
| `PARTICLES_PER_CLICK` | 25 | Number of particles per click |
| `SPAWN_SPEED` | 300.0 | Initial particle velocity |

### Ideas for Modifications

- Change particle size in the render loop (currently 4x4 pixels)
- Add different particle shapes (circles, triangles)
- Implement particle trails
- Add keyboard controls to adjust gravity in real-time
- Create different spawn patterns (streams, explosions, fountains)
- Add particle-to-particle collision
