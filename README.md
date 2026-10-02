# Particle System

An interactive 2-D particle sandbox in C++ and SDL2. Click to spray particles, then shape
their motion with gravity wells, a magnetic field and a vortex. The physics comes straight
from first-year mechanics and linear algebra, and the source is written as a commented,
step-by-step tutorial.

## Controls

| Input | Action |
|---|---|
| Left click (hold to spray) | Spawn bursts of 25 particles and shift the background hue |
| Right click | Place a gravity well |
| Middle click | Remove all gravity wells |
| `M` | Toggle the magnetic field |
| `V` | Toggle the vortex |
| Drag window edges | Resize; the rendering scales with the window |
| `Esc` | Quit |

An on-screen counter tracks particles **created**, particles **destroyed** (swallowed by
wells) and **clicks**.

## Physics

Each frame advances every particle by the measured frame time Δt. Velocity is updated
first, then position (semi-implicit Euler).

| Effect | Model |
|---|---|
| Gravity | Constant downward acceleration, 500 px/s² |
| Gravity wells | Inverse-square attraction a = k / r², with r² clamped at 100 px² to avoid the singularity. Particles within 15 px are absorbed and add screen shake. |
| Magnetic field | Lorentz force F = q v × B with B out of the screen, so (Fx, Fy) = qB(vy, −vx). Each particle gets a random charge of ±1, so the two charges curve in opposite directions. |
| Vortex | Tangential push around the screen centre. The tangent is the radial vector rotated 90° with a rotation matrix. |
| Walls | Reflect the normal velocity, keeping 70% of it |
| Lifetime | 3 s, fading out linearly |

**Known limitation:** the magnetic update rotates the velocity with an explicit Euler step,
which multiplies the speed by √(1 + (qBΔt)²) each frame instead of conserving it. With the
default settings, particles in the magnetic field slowly gain energy. A Boris-style or exact
rotation update would conserve speed exactly.

## Rendering

- Particles are coloured by speed: blue (slow) → cyan → yellow → orange → red (fast, ≥ 800 px/s).
- Each particle leaves a fading 8-point trail stored in a circular buffer.
- Text and digits are drawn with a hand-made bitmap font and seven-segment digits, with no
  font library.

## Build and run

Requires a C++ compiler and SDL2 (`brew install sdl2`, `apt install libsdl2-dev` or
`dnf install SDL2-devel`).

```bash
make quick          # compile and run ./particles
make                # build ParticleSystem.app (macOS app bundle with generated icon)
make run            # build the app bundle and open it
make clean          # remove the executable (cleanall also removes the app bundle)
```

Or compile by hand:

```bash
g++ -o particles particles.cpp $(sdl2-config --cflags --libs)
```

`generate_icon.py` draws the macOS app icon. `build.sh` is a shell-script alternative to
`make`.

## Tuning

All parameters are constants at the top of [particles.cpp](particles.cpp): gravity,
bounce damping, lifetime, particles per click, spawn speed, well strength and radii,
magnetic field strength, vortex strength, trail length and screen-shake settings.

## License

[MIT](LICENSE)
