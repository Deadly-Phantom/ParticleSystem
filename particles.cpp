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

// Random number generator (modern C++ way)
std::random_device rd;                                   // Gets random seed from hardware
std::mt19937 gen(rd());                                  // Mersenne Twister algorithm
std::uniform_real_distribution<> angleDist(0, 2 * M_PI); // Random angle 0-360 degrees (in radians)
std::uniform_real_distribution<> speedDist(0.5, 1.5);    // Speed multiplier
std::uniform_int_distribution<> colorDist(100, 255);     // Bright colors only

// ============================================
// STEP 5: SPAWN FUNCTION
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
// STEP 6: UPDATE FUNCTION
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
// STEP 7: MAIN FUNCTION
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
        "Particle System - Click to spawn!",
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
    std::cout << "Click anywhere to spawn particles.\n";
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
            // Mouse button clicked
            else if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                // Get mouse position and spawn particles there
                int mouseX = event.button.x;
                int mouseY = event.button.y;
                spawnParticles(mouseX, mouseY);
            }
        }

        // --- UPDATE ---
        // Move particles, apply physics
        updateParticles(deltaTime);

        // --- RENDER ---

        // Clear screen to dark gray
        SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
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
