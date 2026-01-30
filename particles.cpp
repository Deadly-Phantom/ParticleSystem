// ============================================
// PARTICLE SYSTEM - Step by Step Tutorial
// ============================================

// STEP 1: INCLUDES
// These are like importing libraries in other languages
#include <SDL.h>    // SDL2 - handles window, graphics, input
#include <vector>   // Dynamic array that can grow/shrink
#include <random>   // Modern C++ random number generation
#include <cmath>    // Math functions (we'll use for angles)
#include <iostream> // For printing to console

// ============================================
// STEP 2: CONSTANTS
// These control the behavior of our particle system
// ============================================

// Window dimensions (not const - can be resized)
int WINDOW_WIDTH = 800;
int WINDOW_HEIGHT = 600;

// Base dimensions for scaling (design size)
const int BASE_WIDTH = 800;
const int BASE_HEIGHT = 600;

// Physics constants
const float GRAVITY = 500.0f;         // Pixels per second^2 (downward acceleration)
const float BOUNCE_DAMPING = 0.7f;    // Energy lost on bounce (0.7 = keeps 70% velocity)
const float PARTICLE_LIFETIME = 3.0f; // Seconds before particle dies

// Spawn settings
const int PARTICLES_PER_CLICK = 25; // How many particles spawn per click
const float SPAWN_SPEED = 300.0f;   // Initial speed of particles

// Background color settings
const float HUE_INCREMENT = 6.0f; // Degrees per click (60 clicks = full rainbow)

// Gravity well settings
const float GRAVITY_WELL_STRENGTH = 50000000.0f; // Pull force (higher = stronger attraction)
const float GRAVITY_WELL_RADIUS = 25.0f;         // Visual size of the well
const float GRAVITY_WELL_SUCK_RADIUS = 15.0f;    // Particles within this distance get consumed

// Magnetic field settings (PHY180: Lorentz force F = qv × B)
// With B perpendicular to screen: Fx = q*B*vy, Fy = -q*B*vx
const float MAGNETIC_FIELD_STRENGTH = 8.0f; // Tesla (magnetic field pointing out of screen)

// Visual effects settings
const int TRAIL_LENGTH = 8;          // Number of trail segments per particle
const float TRAIL_SPACING = 0.02f;   // Seconds between trail updates
const float MAX_SPEED = 800.0f;      // Speed at which particles are "hottest"
const float SHAKE_DECAY = 8.0f;      // How fast screen shake fades
const float SHAKE_INTENSITY = 0.02f; // Shake amount per particle sucked

// ============================================
// STEP 3: PARTICLE STRUCT
// A struct is like a container that holds related data together
// Each particle needs to know its position, velocity, color, etc.
// ============================================

struct Particle
{
    // Position (where the particle is on screen)
    float x, y;

    // Velocity (how fast and which direction it's moving)
    // vx = horizontal speed, vy = vertical speed
    float vx, vy;

    // Base color (RGB values 0-255) - will be modified by velocity
    Uint8 r, g, b;

    // Alpha (transparency: 1.0 = fully visible, 0.0 = invisible)
    float alpha;

    // Life remaining (seconds until this particle dies)
    float life;

    // Electric charge (+1 or -1) for magnetic force (PHY180: F = qv × B)
    float charge;

    // Trail history (previous positions for afterglow effect)
    float trailX[TRAIL_LENGTH];
    float trailY[TRAIL_LENGTH];
    int trailIndex;   // Current position in circular buffer
    float trailTimer; // Time until next trail update
};

// Gravity well - attracts particles toward it
struct GravityWell
{
    float x, y; // Position
};

// ============================================
// STEP 4: GLOBAL VARIABLES
// These are accessible from anywhere in the program
// ============================================

// Vector = dynamic array. It can hold any number of particles
// and automatically grows when we add more
std::vector<Particle> particles;

// Gravity wells
std::vector<GravityWell> gravityWells;

// Background color (hue in degrees, 0-360)
float backgroundHue = 0.0f;

// Mouse state for continuous spawning
bool mouseHeld = false;
int mouseX = 0;
int mouseY = 0;

// Stats tracking
int totalParticlesCreated = 0;
int particlesSucked = 0;
int totalClicks = 0;

// Screen shake effect
float screenShake = 0.0f;

// Magnetic field toggle (press 'M' to toggle)
bool magneticFieldEnabled = true;

// Random number generator (modern C++ way)
std::random_device rd;                                   // Gets random seed from hardware
std::mt19937 gen(rd());                                  // Mersenne Twister algorithm
std::uniform_real_distribution<> angleDist(0, 2 * M_PI); // Random angle 0-360 degrees (in radians)
std::uniform_real_distribution<> speedDist(0.5, 1.5);    // Speed multiplier
std::uniform_int_distribution<> colorDist(100, 255);     // Bright colors only
std::uniform_int_distribution<> chargeDist(0, 1);        // Random charge: 0 -> -1, 1 -> +1

// ============================================
// STEP 5: HSL TO RGB CONVERSION
// Converts Hue (0-360), Saturation (0-1), Lightness (0-1) to RGB (0-255)
// This makes it easy to cycle through rainbow colors
// ============================================

void hslToRgb(float h, float s, float l, Uint8 &r, Uint8 &g, Uint8 &b)
{
    // Normalize hue to 0-1 range
    h = fmod(h, 360.0f) / 360.0f;

    float c = (1.0f - fabs(2.0f * l - 1.0f)) * s; // Chroma
    float x = c * (1.0f - fabs(fmod(h * 6.0f, 2.0f) - 1.0f));
    float m = l - c / 2.0f;

    float rf, gf, bf;

    if (h < 1.0f / 6.0f)
    {
        rf = c;
        gf = x;
        bf = 0;
    }
    else if (h < 2.0f / 6.0f)
    {
        rf = x;
        gf = c;
        bf = 0;
    }
    else if (h < 3.0f / 6.0f)
    {
        rf = 0;
        gf = c;
        bf = x;
    }
    else if (h < 4.0f / 6.0f)
    {
        rf = 0;
        gf = x;
        bf = c;
    }
    else if (h < 5.0f / 6.0f)
    {
        rf = x;
        gf = 0;
        bf = c;
    }
    else
    {
        rf = c;
        gf = 0;
        bf = x;
    }

    r = static_cast<Uint8>((rf + m) * 255);
    g = static_cast<Uint8>((gf + m) * 255);
    b = static_cast<Uint8>((bf + m) * 255);
}

// ============================================
// STEP 6: DRAWING HELPER FUNCTIONS
// Circle drawing, numbers, and text labels
// ============================================

// Draw a filled circle (for gravity wells)
void drawFilledCircle(SDL_Renderer *renderer, int centerX, int centerY, int radius)
{
    for (int y = -radius; y <= radius; y++)
    {
        for (int x = -radius; x <= radius; x++)
        {
            if (x * x + y * y <= radius * radius)
            {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

// Draw a circle outline (for glow rings)
void drawCircleOutline(SDL_Renderer *renderer, int centerX, int centerY, int radius, int thickness)
{
    for (int y = -radius - thickness; y <= radius + thickness; y++)
    {
        for (int x = -radius - thickness; x <= radius + thickness; x++)
        {
            int distSq = x * x + y * y;
            int innerSq = (radius - thickness) * (radius - thickness);
            int outerSq = (radius + thickness) * (radius + thickness);
            if (distSq >= innerSq && distSq <= outerSq)
            {
                SDL_RenderDrawPoint(renderer, centerX + x, centerY + y);
            }
        }
    }
}

// Simple 3x5 pixel font for labels (compact)
// Each letter is a 3-wide, 5-tall bitmap stored as 5 rows of 3 bits
const uint8_t MINI_FONT[26][5] = {
    {0b010, 0b101, 0b111, 0b101, 0b101}, // A
    {0b110, 0b101, 0b110, 0b101, 0b110}, // B
    {0b011, 0b100, 0b100, 0b100, 0b011}, // C
    {0b110, 0b101, 0b101, 0b101, 0b110}, // D
    {0b111, 0b100, 0b110, 0b100, 0b111}, // E
    {0b111, 0b100, 0b110, 0b100, 0b100}, // F
    {0b011, 0b100, 0b101, 0b101, 0b011}, // G
    {0b101, 0b101, 0b111, 0b101, 0b101}, // H
    {0b111, 0b010, 0b010, 0b010, 0b111}, // I
    {0b001, 0b001, 0b001, 0b101, 0b010}, // J
    {0b101, 0b110, 0b100, 0b110, 0b101}, // K
    {0b100, 0b100, 0b100, 0b100, 0b111}, // L
    {0b101, 0b111, 0b111, 0b101, 0b101}, // M
    {0b101, 0b111, 0b111, 0b111, 0b101}, // N
    {0b010, 0b101, 0b101, 0b101, 0b010}, // O
    {0b110, 0b101, 0b110, 0b100, 0b100}, // P
    {0b010, 0b101, 0b101, 0b110, 0b011}, // Q
    {0b110, 0b101, 0b110, 0b101, 0b101}, // R
    {0b011, 0b100, 0b010, 0b001, 0b110}, // S
    {0b111, 0b010, 0b010, 0b010, 0b010}, // T
    {0b101, 0b101, 0b101, 0b101, 0b011}, // U
    {0b101, 0b101, 0b101, 0b101, 0b010}, // V
    {0b101, 0b101, 0b111, 0b111, 0b101}, // W
    {0b101, 0b101, 0b010, 0b101, 0b101}, // X
    {0b101, 0b101, 0b010, 0b010, 0b010}, // Y
    {0b111, 0b001, 0b010, 0b100, 0b111}, // Z
};

void drawLetter(SDL_Renderer *renderer, char c, int x, int y, int scale)
{
    int index = -1;
    if (c >= 'A' && c <= 'Z')
        index = c - 'A';
    else if (c >= 'a' && c <= 'z')
        index = c - 'a';
    else
        return;

    for (int row = 0; row < 5; row++)
    {
        for (int col = 0; col < 3; col++)
        {
            if (MINI_FONT[index][row] & (0b100 >> col))
            {
                SDL_Rect pixel = {x + col * scale, y + row * scale, scale, scale};
                SDL_RenderFillRect(renderer, &pixel);
            }
        }
    }
}

void drawText(SDL_Renderer *renderer, const char *text, int x, int y, int scale)
{
    int cursorX = x;
    for (int i = 0; text[i] != '\0'; i++)
    {
        if (text[i] == ' ')
        {
            cursorX += scale * 2;
        }
        else
        {
            drawLetter(renderer, text[i], cursorX, y, scale);
            cursorX += scale * 4; // 3 pixels + 1 spacing
        }
    }
}

// 7-segment display for numbers
// Segment layout:     0
//                    ---
//                 1 |   | 2
//                    ---  <- 3
//                 4 |   | 5
//                    ---
//                     6

// Which segments are on for each digit (0-9)
const bool DIGIT_SEGMENTS[10][7] = {
    {1, 1, 1, 0, 1, 1, 1}, // 0
    {0, 0, 1, 0, 0, 1, 0}, // 1
    {1, 0, 1, 1, 1, 0, 1}, // 2
    {1, 0, 1, 1, 0, 1, 1}, // 3
    {0, 1, 1, 1, 0, 1, 0}, // 4
    {1, 1, 0, 1, 0, 1, 1}, // 5
    {1, 1, 0, 1, 1, 1, 1}, // 6
    {1, 0, 1, 0, 0, 1, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}, // 9
};

void drawDigit(SDL_Renderer *renderer, int digit, int x, int y, int scale)
{
    if (digit < 0 || digit > 9)
        return;

    int w = scale;     // Segment width
    int h = scale * 2; // Segment height (for vertical segments)
    int t = scale / 3; // Thickness

    // Segment positions relative to top-left of digit
    SDL_Rect segments[7] = {
        {x + t, y, w, t},                 // 0: top
        {x, y + t, t, h},                 // 1: top-left
        {x + w + t, y + t, t, h},         // 2: top-right
        {x + t, y + h + t, w, t},         // 3: middle
        {x, y + h + 2 * t, t, h},         // 4: bottom-left
        {x + w + t, y + h + 2 * t, t, h}, // 5: bottom-right
        {x + t, y + 2 * h + 2 * t, w, t}, // 6: bottom
    };

    for (int i = 0; i < 7; i++)
    {
        if (DIGIT_SEGMENTS[digit][i])
        {
            SDL_RenderFillRect(renderer, &segments[i]);
        }
    }
}

// Draw a full number (multiple digits)
void drawNumber(SDL_Renderer *renderer, int number, int x, int y, int scale)
{
    if (number == 0)
    {
        drawDigit(renderer, 0, x, y, scale);
        return;
    }

    // Count digits to know where to start
    int temp = number;
    int digitCount = 0;
    while (temp > 0)
    {
        digitCount++;
        temp /= 10;
    }

    // Draw from right to left
    int digitWidth = scale * 2 + scale / 2; // Width of one digit plus spacing
    int currentX = x + (digitCount - 1) * digitWidth;

    while (number > 0)
    {
        int digit = number % 10;
        drawDigit(renderer, digit, currentX, y, scale);
        number /= 10;
        currentX -= digitWidth;
    }
}

// ============================================
// STEP 7: SPAWN FUNCTION
// Creates a burst of particles at the given position
// ============================================

void spawnParticles(float spawnX, float spawnY)
{
    for (int i = 0; i < PARTICLES_PER_CLICK; i++)
    {
        Particle p;

        // Set position to where we clicked
        p.x = spawnX;
        p.y = spawnY;

        // Random direction (angle in radians)
        float angle = angleDist(gen);

        // Random speed (varies the burst pattern)
        float speed = SPAWN_SPEED * speedDist(gen);

        // Convert angle + speed into vx and vy using trigonometry:
        // cos(angle) gives x component, sin(angle) gives y component
        p.vx = cos(angle) * speed;
        p.vy = sin(angle) * speed;

        // Random bright color
        p.r = colorDist(gen);
        p.g = colorDist(gen);
        p.b = colorDist(gen);

        // Start fully visible
        p.alpha = 1.0f;

        // Full lifetime
        p.life = PARTICLE_LIFETIME;

        // Random charge (+1 or -1) for magnetic deflection
        p.charge = chargeDist(gen) == 1 ? 1.0f : -1.0f;

        // Initialize trail history (all positions start at spawn point)
        for (int t = 0; t < TRAIL_LENGTH; t++)
        {
            p.trailX[t] = spawnX;
            p.trailY[t] = spawnY;
        }
        p.trailIndex = 0;
        p.trailTimer = 0.0f;

        // Add to our vector of particles
        particles.push_back(p);
        totalParticlesCreated++;
    }
}

// ============================================
// STEP 8: UPDATE FUNCTION
// Called every frame to move particles and apply physics
// deltaTime = seconds since last frame (usually ~0.016 for 60fps)
// ============================================

void updateParticles(float deltaTime)
{
    // Loop through all particles
    // Using iterator because we might remove particles while looping
    for (auto it = particles.begin(); it != particles.end();)
    {
        // Get reference to current particle (so we can modify it)
        Particle &p = *it;
        bool suckedByWell = false;

        // --- PHYSICS ---

        // Apply gravity (accelerate downward)
        // velocity = velocity + acceleration * time
        p.vy += GRAVITY * deltaTime;

        // Apply magnetic force (PHY180: Lorentz force F = qv × B)
        // With B pointing out of screen (z-direction):
        //   F = q(v × B) = q(vy*B, -vx*B, 0)
        // This creates circular motion! Cyclotron radius r = mv/(qB)
        if (magneticFieldEnabled)
        {
            float magForceX = p.charge * MAGNETIC_FIELD_STRENGTH * p.vy;
            float magForceY = -p.charge * MAGNETIC_FIELD_STRENGTH * p.vx;
            p.vx += magForceX * deltaTime;
            p.vy += magForceY * deltaTime;
        }

        // Apply gravity well attraction
        for (const GravityWell &well : gravityWells)
        {
            // Vector from particle to well
            float dx = well.x - p.x;
            float dy = well.y - p.y;

            // Distance squared (avoid sqrt for efficiency)
            float distSq = dx * dx + dy * dy;
            float dist = sqrt(distSq);

            // Check if particle is close enough to be sucked in
            if (dist < GRAVITY_WELL_SUCK_RADIUS)
            {
                suckedByWell = true;
                particlesSucked++;
                break;
            }

            // Avoid division by zero and limit max force when very close
            if (distSq < 100.0f)
                distSq = 100.0f;

            // Gravitational force: F = strength / distance^2
            float force = GRAVITY_WELL_STRENGTH / distSq;

            // Apply force in direction of well
            p.vx += (dx / dist) * force * deltaTime;
            p.vy += (dy / dist) * force * deltaTime;
        }

        // If sucked by well, remove particle and add screen shake
        if (suckedByWell)
        {
            screenShake += SHAKE_INTENSITY;
            if (screenShake > 5.0f)
                screenShake = 5.0f; // Cap max shake
            it = particles.erase(it);
            continue;
        }

        // Update trail history (circular buffer)
        p.trailTimer += deltaTime;
        if (p.trailTimer >= TRAIL_SPACING)
        {
            p.trailTimer = 0.0f;
            p.trailX[p.trailIndex] = p.x;
            p.trailY[p.trailIndex] = p.y;
            p.trailIndex = (p.trailIndex + 1) % TRAIL_LENGTH;
        }

        // Update position based on velocity
        // position = position + velocity * time
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;

        // --- BOUNCE OFF WALLS ---

        // Left wall
        if (p.x < 0)
        {
            p.x = 0;
            p.vx = -p.vx * BOUNCE_DAMPING;
        }
        // Right wall
        if (p.x > WINDOW_WIDTH)
        {
            p.x = WINDOW_WIDTH;
            p.vx = -p.vx * BOUNCE_DAMPING;
        }
        // Top wall
        if (p.y < 0)
        {
            p.y = 0;
            p.vy = -p.vy * BOUNCE_DAMPING;
        }
        // Bottom wall (floor)
        if (p.y > WINDOW_HEIGHT)
        {
            p.y = WINDOW_HEIGHT;
            p.vy = -p.vy * BOUNCE_DAMPING;
        }

        // --- LIFETIME & FADING ---

        // Decrease remaining life
        p.life -= deltaTime;

        // Fade out: alpha decreases as life decreases
        p.alpha = p.life / PARTICLE_LIFETIME;

        // --- REMOVE DEAD PARTICLES ---

        if (p.life <= 0)
        {
            // erase() removes this particle and returns iterator to next
            it = particles.erase(it);
        }
        else
        {
            // Move to next particle
            ++it;
        }
    }
}

// ============================================
// STEP 9: MAIN FUNCTION
// Entry point - sets up SDL and runs the game loop
// ============================================

int main(int argc, char *argv[])
{
    // --- INITIALIZE SDL ---

    // SDL_Init starts up the SDL library
    // SDL_INIT_VIDEO means we want graphics/window support
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init Error: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Create the window
    // Parameters: title, x position, y position, width, height, flags
    // SDL_WINDOWPOS_CENTERED = center the window on screen
    // SDL_WINDOW_RESIZABLE = allow user to resize the window
    SDL_Window *window = SDL_CreateWindow(
        "Particle System",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        SDL_WINDOW_RESIZABLE);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    // Create renderer (this is what actually draws to the window)
    // SDL_RENDERER_ACCELERATED = use GPU for faster rendering
    // SDL_RENDERER_PRESENTVSYNC = sync to monitor refresh rate (prevents tearing)
    SDL_Renderer *renderer = SDL_CreateRenderer(
        window, -1,
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer)
    {
        std::cerr << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // Enable alpha blending (so transparent particles work correctly)
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    std::cout << "Particle System running!\n";
    std::cout << "Left-click: Spawn particles (hold to spray)\n";
    std::cout << "Right-click: Place gravity well\n";
    std::cout << "Middle-click: Clear all gravity wells\n";
    std::cout << "M: Toggle magnetic field (Lorentz force F = qv × B)\n";
    std::cout << "Drag window edges to resize\n";
    std::cout << "ESC: Quit\n";

    // --- GAME LOOP VARIABLES ---

    bool running = true;
    Uint32 lastTime = SDL_GetTicks(); // Milliseconds since SDL started

    // --- MAIN GAME LOOP ---
    // This runs continuously until the user quits

    while (running)
    {
        // --- CALCULATE DELTA TIME ---
        // How much time passed since last frame (in seconds)
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f; // Convert ms to seconds
        lastTime = currentTime;

        // --- EVENT HANDLING ---
        // Process all pending events (mouse clicks, key presses, etc.)

        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            // Window close button clicked
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            // Key pressed
            else if (event.type == SDL_KEYDOWN)
            {
                // ESC key quits
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                // M key toggles magnetic field
                else if (event.key.keysym.sym == SDLK_m)
                {
                    magneticFieldEnabled = !magneticFieldEnabled;
                }
            }
            // Mouse button pressed
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                mouseX = event.button.x;
                mouseY = event.button.y;

                // Left click - spawn particles
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mouseHeld = true;
                    totalClicks++;

                    // Spawn particles and advance background color
                    spawnParticles(mouseX, mouseY);
                    backgroundHue += HUE_INCREMENT;
                    if (backgroundHue >= 360.0f)
                        backgroundHue -= 360.0f;
                }
                // Right click - place gravity well
                else if (event.button.button == SDL_BUTTON_RIGHT)
                {
                    GravityWell well;
                    well.x = mouseX;
                    well.y = mouseY;
                    gravityWells.push_back(well);
                }
                // Middle click - clear all gravity wells
                else if (event.button.button == SDL_BUTTON_MIDDLE)
                {
                    gravityWells.clear();
                }
            }
            // Mouse button released - stop spawning (left button only)
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                if (event.button.button == SDL_BUTTON_LEFT)
                {
                    mouseHeld = false;
                }
            }
            // Mouse moved - update position for continuous spawning
            else if (event.type == SDL_MOUSEMOTION)
            {
                mouseX = event.motion.x;
                mouseY = event.motion.y;
            }
            // Window resized
            else if (event.type == SDL_WINDOWEVENT)
            {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED)
                {
                    WINDOW_WIDTH = event.window.data1;
                    WINDOW_HEIGHT = event.window.data2;
                }
            }
        }

        // Continuous spawning while mouse is held (background only changes on initial click)
        if (mouseHeld)
        {
            spawnParticles(mouseX, mouseY);
        }

        // --- UPDATE ---
        // Move particles, apply physics
        updateParticles(deltaTime);

        // Decay screen shake over time
        if (screenShake > 0)
        {
            screenShake -= SHAKE_DECAY * deltaTime;
            if (screenShake < 0)
                screenShake = 0;
        }

        // --- RENDER ---

        // Calculate scale factor based on window size
        float scaleX = static_cast<float>(WINDOW_WIDTH) / BASE_WIDTH;
        float scaleY = static_cast<float>(WINDOW_HEIGHT) / BASE_HEIGHT;
        float scale = (scaleX + scaleY) / 2.0f; // Average scale

        // Clear screen with rainbow background color
        // Using low lightness (0.15) to keep it dark but colorful
        Uint8 bgR, bgG, bgB;
        hslToRgb(backgroundHue, 0.6f, 0.15f, bgR, bgG, bgB);
        SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 255);
        SDL_RenderClear(renderer);

        // Calculate screen shake offset
        int shakeOffsetX = 0;
        int shakeOffsetY = 0;
        if (screenShake > 0)
        {
            std::uniform_real_distribution<> shakeDist(-screenShake, screenShake);
            shakeOffsetX = static_cast<int>(shakeDist(gen) * scale);
            shakeOffsetY = static_cast<int>(shakeDist(gen) * scale);
        }

        // Scaled particle size
        int particleRadius = static_cast<int>(3 * scale);
        if (particleRadius < 2)
            particleRadius = 2;

        // Draw each particle with trails and velocity coloring
        for (const Particle &p : particles)
        {
            // Calculate speed for velocity-based coloring
            float speed = sqrt(p.vx * p.vx + p.vy * p.vy);
            float speedRatio = speed / MAX_SPEED;
            if (speedRatio > 1.0f)
                speedRatio = 1.0f;

            // Color gradient: blue (cold/slow) -> cyan -> yellow -> orange -> red (hot/fast)
            Uint8 velR, velG, velB;
            if (speedRatio < 0.25f)
            {
                // Blue to cyan
                float t = speedRatio / 0.25f;
                velR = static_cast<Uint8>(50 * t);
                velG = static_cast<Uint8>(100 + 155 * t);
                velB = 255;
            }
            else if (speedRatio < 0.5f)
            {
                // Cyan to yellow
                float t = (speedRatio - 0.25f) / 0.25f;
                velR = static_cast<Uint8>(50 + 205 * t);
                velG = 255;
                velB = static_cast<Uint8>(255 - 255 * t);
            }
            else if (speedRatio < 0.75f)
            {
                // Yellow to orange
                float t = (speedRatio - 0.5f) / 0.25f;
                velR = 255;
                velG = static_cast<Uint8>(255 - 100 * t);
                velB = 0;
            }
            else
            {
                // Orange to red
                float t = (speedRatio - 0.75f) / 0.25f;
                velR = 255;
                velG = static_cast<Uint8>(155 - 155 * t);
                velB = 0;
            }

            // Draw trail (fading afterimages) - oldest to newest
            for (int t = 0; t < TRAIL_LENGTH; t++)
            {
                // Get trail position from circular buffer (oldest first)
                int idx = (p.trailIndex + t) % TRAIL_LENGTH;
                int trailX = static_cast<int>(p.trailX[idx]) + shakeOffsetX;
                int trailY = static_cast<int>(p.trailY[idx]) + shakeOffsetY;

                // Trail gets more transparent and smaller the older it is
                float trailAge = static_cast<float>(t) / TRAIL_LENGTH;
                Uint8 trailAlpha = static_cast<Uint8>(p.alpha * 80 * (1.0f - trailAge));
                int trailRadius = static_cast<int>(particleRadius * (0.3f + 0.5f * trailAge));

                if (trailAlpha > 5 && trailRadius > 0)
                {
                    SDL_SetRenderDrawColor(renderer, velR, velG, velB, trailAlpha);
                    drawFilledCircle(renderer, trailX, trailY, trailRadius);
                }
            }

            // Draw main particle as a circle with velocity color
            int px = static_cast<int>(p.x) + shakeOffsetX;
            int py = static_cast<int>(p.y) + shakeOffsetY;
            Uint8 alphaValue = static_cast<Uint8>(p.alpha * 255);
            SDL_SetRenderDrawColor(renderer, velR, velG, velB, alphaValue);
            drawFilledCircle(renderer, px, py, particleRadius);
        }

        // Scaled gravity well size
        int wellRadius = static_cast<int>(GRAVITY_WELL_RADIUS * scale);
        int glowOuter = static_cast<int>(8 * scale);
        int glowInner = static_cast<int>(4 * scale);
        int coreSize = static_cast<int>(3 * scale);
        if (coreSize < 2)
            coreSize = 2;

        // Draw gravity wells (cyberpunk circular style)
        for (const GravityWell &well : gravityWells)
        {
            int cx = static_cast<int>(well.x) + shakeOffsetX;
            int cy = static_cast<int>(well.y) + shakeOffsetY;

            // Outer cyan glow
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 60);
            drawFilledCircle(renderer, cx, cy, wellRadius + glowOuter);

            // Purple glow ring
            SDL_SetRenderDrawColor(renderer, 180, 0, 255, 100);
            drawFilledCircle(renderer, cx, cy, wellRadius + glowInner);

            // Bright cyan ring
            SDL_SetRenderDrawColor(renderer, 0, 255, 255, 180);
            drawCircleOutline(renderer, cx, cy, wellRadius, static_cast<int>(2 * scale));

            // Dark center (black hole effect)
            SDL_SetRenderDrawColor(renderer, 5, 5, 15, 255);
            drawFilledCircle(renderer, cx, cy, wellRadius - glowInner);

            // Tiny bright core
            SDL_SetRenderDrawColor(renderer, 120, 0, 200, 200);
            drawFilledCircle(renderer, cx, cy, coreSize);
        }

        // --- DRAW STATS OVERLAY ---
        // Compact stats in top-left corner with labels

        int textScale = static_cast<int>(2 * scale);
        int numScale = static_cast<int>(3 * scale);
        if (textScale < 1)
            textScale = 1;
        if (numScale < 2)
            numScale = 2;

        int labelX = static_cast<int>(10 * scale);
        int numberX = static_cast<int>(100 * scale); // All numbers aligned here
        int statY = static_cast<int>(10 * scale);
        int statSpacing = static_cast<int>(22 * scale);

        // CREATED - total particles spawned (cyan/blue - cold, new)
        SDL_SetRenderDrawColor(renderer, 50, 200, 255, 220);
        drawText(renderer, "CREATED", labelX, statY + 2, textScale);
        drawNumber(renderer, totalParticlesCreated, numberX, statY, numScale);
        statY += statSpacing;

        // DESTROYED - particles consumed by wells (red/orange - hot, fast)
        SDL_SetRenderDrawColor(renderer, 255, 100, 50, 220);
        drawText(renderer, "DESTROYED", labelX, statY + 2, textScale);
        drawNumber(renderer, particlesSucked, numberX, statY, numScale);
        statY += statSpacing;

        // CLICKS - total clicks (green)
        SDL_SetRenderDrawColor(renderer, 100, 255, 100, 220);
        drawText(renderer, "CLICKS", labelX, statY + 2, textScale);
        drawNumber(renderer, totalClicks, numberX, statY, numScale);

        // Show what we drew (swap buffers)
        SDL_RenderPresent(renderer);
    }

    // --- CLEANUP ---
    // Always clean up SDL resources when done

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
