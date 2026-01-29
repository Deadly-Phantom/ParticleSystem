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

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 600;

// Physics constants
const float GRAVITY = 500.0f;         // Pixels per second^2 (downward acceleration)
const float BOUNCE_DAMPING = 0.7f;    // Energy lost on bounce (0.7 = keeps 70% velocity)
const float PARTICLE_LIFETIME = 3.0f; // Seconds before particle dies

// Spawn settings
const int PARTICLES_PER_CLICK = 25; // How many particles spawn per click
const float SPAWN_SPEED = 300.0f;   // Initial speed of particles

// Background color settings
const float HUE_INCREMENT = 6.0f; // Degrees per click (60 clicks = full rainbow)

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

    // Color (RGB values 0-255)
    Uint8 r, g, b;

    // Alpha (transparency: 1.0 = fully visible, 0.0 = invisible)
    float alpha;

    // Life remaining (seconds until this particle dies)
    float life;
};

// ============================================
// STEP 4: GLOBAL VARIABLES
// These are accessible from anywhere in the program
// ============================================

// Vector = dynamic array. It can hold any number of particles
// and automatically grows when we add more
std::vector<Particle> particles;

// Background color (hue in degrees, 0-360)
float backgroundHue = 0.0f;

// Mouse state for continuous spawning
bool mouseHeld = false;
int mouseX = 0;
int mouseY = 0;

// Random number generator (modern C++ way)
std::random_device rd;                                   // Gets random seed from hardware
std::mt19937 gen(rd());                                  // Mersenne Twister algorithm
std::uniform_real_distribution<> angleDist(0, 2 * M_PI); // Random angle 0-360 degrees (in radians)
std::uniform_real_distribution<> speedDist(0.5, 1.5);    // Speed multiplier
std::uniform_int_distribution<> colorDist(100, 255);     // Bright colors only

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
// STEP 6: SPAWN FUNCTION
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

        // Add to our vector of particles
        particles.push_back(p);
    }
}

// ============================================
// STEP 7: UPDATE FUNCTION
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

        // --- PHYSICS ---

        // Apply gravity (accelerate downward)
        // velocity = velocity + acceleration * time
        p.vy += GRAVITY * deltaTime;

        // Update position based on velocity
        // position = position + velocity * time
        p.x += p.vx * deltaTime;
        p.y += p.vy * deltaTime;

        // --- BOUNCE OFF WALLS ---

        // Left wall
        if (p.x < 0)
        {
            p.x = 0;                       // Put back inside
            p.vx = -p.vx * BOUNCE_DAMPING; // Reverse and reduce velocity
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
// STEP 8: MAIN FUNCTION
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
    SDL_Window *window = SDL_CreateWindow(
        "Particle System - Click or drag to spawn!",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        0 // No special flags
    );

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
    std::cout << "Click or hold mouse button to spawn particles.\n";
    std::cout << "Each click changes the background color!\n";
    std::cout << "Press ESC or close window to quit.\n";

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
            }
            // Mouse button pressed - start spawning
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                mouseHeld = true;
                mouseX = event.button.x;
                mouseY = event.button.y;

                // Spawn particles and advance background color
                spawnParticles(mouseX, mouseY);
                backgroundHue += HUE_INCREMENT;
                if (backgroundHue >= 360.0f)
                    backgroundHue -= 360.0f;
            }
            // Mouse button released - stop spawning
            else if (event.type == SDL_MOUSEBUTTONUP)
            {
                mouseHeld = false;
            }
            // Mouse moved - update position for continuous spawning
            else if (event.type == SDL_MOUSEMOTION)
            {
                mouseX = event.motion.x;
                mouseY = event.motion.y;
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

        // --- RENDER ---

        // Clear screen with rainbow background color
        // Using low lightness (0.15) to keep it dark but colorful
        Uint8 bgR, bgG, bgB;
        hslToRgb(backgroundHue, 0.6f, 0.15f, bgR, bgG, bgB);
        SDL_SetRenderDrawColor(renderer, bgR, bgG, bgB, 255);
        SDL_RenderClear(renderer);

        // Draw each particle
        for (const Particle &p : particles)
        {
            // Set color with current alpha
            Uint8 alphaValue = static_cast<Uint8>(p.alpha * 255);
            SDL_SetRenderDrawColor(renderer, p.r, p.g, p.b, alphaValue);

            // Draw as a 4x4 rectangle (small square)
            SDL_Rect rect = {
                static_cast<int>(p.x) - 2, // Center the rect on particle position
                static_cast<int>(p.y) - 2,
                4, 4 // Width and height
            };
            SDL_RenderFillRect(renderer, &rect);
        }

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
