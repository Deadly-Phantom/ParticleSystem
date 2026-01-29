#!/usr/bin/env python3
"""Generate a gravity well icon for the Particle System app"""

import subprocess
import os
import math

def create_ppm_icon(size):
    """Create a PPM image of the gravity well icon"""
    pixels = []
    center = size // 2

    for y in range(size):
        row = []
        for x in range(size):
            # Distance from center
            dx = x - center
            dy = y - center
            dist = math.sqrt(dx * dx + dy * dy)

            # Normalized distance (0 at center, 1 at edge)
            max_dist = center * 0.85
            norm_dist = dist / max_dist

            # Background (dark purple-blue gradient)
            if norm_dist > 1.0:
                # Outside the main circle - dark background
                r, g, b = 15, 10, 25
            elif norm_dist > 0.9:
                # Outer edge fade
                fade = (1.0 - norm_dist) / 0.1
                r = int(15 + fade * 25)
                g = int(10 + fade * 15)
                b = int(25 + fade * 35)
            elif norm_dist > 0.75:
                # Outer cyan glow
                intensity = 1.0 - (norm_dist - 0.75) / 0.15
                r = int(40 * intensity)
                g = int(200 * intensity + 55)
                b = int(220 * intensity + 35)
            elif norm_dist > 0.6:
                # Purple glow ring
                intensity = 1.0 - (norm_dist - 0.6) / 0.15
                r = int(120 + 60 * intensity)
                g = int(20 + 80 * intensity)
                b = int(180 + 75 * intensity)
            elif norm_dist > 0.45:
                # Bright cyan ring
                ring_pos = (norm_dist - 0.45) / 0.15
                if 0.3 < ring_pos < 0.7:
                    # Bright part of ring
                    r, g, b = 0, 255, 255
                else:
                    # Fade in/out
                    if ring_pos <= 0.3:
                        fade = ring_pos / 0.3
                    else:
                        fade = (1.0 - ring_pos) / 0.3
                    r = int(100 * fade)
                    g = int(200 + 55 * fade)
                    b = int(220 + 35 * fade)
            elif norm_dist > 0.15:
                # Dark center gradient
                gradient = (norm_dist - 0.15) / 0.3
                r = int(5 + gradient * 15)
                g = int(5 + gradient * 10)
                b = int(15 + gradient * 20)
            else:
                # Bright purple core
                core_intensity = 1.0 - (norm_dist / 0.15)
                r = int(80 + 70 * core_intensity)
                g = int(20 * core_intensity)
                b = int(150 + 105 * core_intensity)

            # Add some particles (small bright dots)
            # Using deterministic positions based on coordinates
            particle_seed = (x * 7 + y * 13) % 100
            if particle_seed < 3 and 0.3 < norm_dist < 0.85:
                # Random bright particle
                angle = math.atan2(dy, dx)
                spiral = (dist / 10 + angle) % (math.pi / 4)
                if spiral < 0.15:
                    colors = [(255, 100, 100), (100, 255, 100), (100, 200, 255), (255, 255, 100)]
                    color = colors[particle_seed % 4]
                    r, g, b = color

            row.append((r, g, b))
        pixels.append(row)

    return pixels

def save_ppm(pixels, filename):
    """Save pixels as PPM file"""
    size = len(pixels)
    with open(filename, 'w') as f:
        f.write(f"P3\n{size} {size}\n255\n")
        for row in pixels:
            for r, g, b in row:
                f.write(f"{r} {g} {b} ")
            f.write("\n")

def main():
    print("Generating Particle System icon...")

    # Create iconset directory
    iconset_dir = "ParticleSystem.iconset"
    os.makedirs(iconset_dir, exist_ok=True)

    # Required sizes for macOS icons
    sizes = [16, 32, 64, 128, 256, 512, 1024]

    for size in sizes:
        print(f"  Creating {size}x{size}...")
        pixels = create_ppm_icon(size)

        ppm_file = f"{iconset_dir}/icon_{size}.ppm"
        save_ppm(pixels, ppm_file)

        # Convert PPM to PNG using sips
        png_file = f"{iconset_dir}/icon_{size}x{size}.png"
        subprocess.run(["sips", "-s", "format", "png", ppm_file, "--out", png_file],
                      capture_output=True)
        os.remove(ppm_file)

        # Create @2x versions for Retina
        if size <= 512:
            retina_size = size * 2
            png_2x = f"{iconset_dir}/icon_{size}x{size}@2x.png"
            # Copy the larger version as @2x
            if retina_size in sizes:
                src = f"{iconset_dir}/icon_{retina_size}x{retina_size}.png"
                if os.path.exists(src):
                    subprocess.run(["cp", src, png_2x])

    # Generate @2x versions that we missed
    for size in [16, 32, 128, 256, 512]:
        retina_size = size * 2
        png_2x = f"{iconset_dir}/icon_{size}x{size}@2x.png"
        if not os.path.exists(png_2x):
            print(f"  Creating {size}x{size}@2x...")
            pixels = create_ppm_icon(retina_size)
            ppm_file = f"{iconset_dir}/temp.ppm"
            save_ppm(pixels, ppm_file)
            subprocess.run(["sips", "-s", "format", "png", ppm_file, "--out", png_2x],
                          capture_output=True)
            os.remove(ppm_file)

    # Remove 64x64 and 1024x1024 as they're not standard iconset sizes
    for f in ["icon_64x64.png", "icon_1024x1024.png"]:
        path = f"{iconset_dir}/{f}"
        if os.path.exists(path):
            os.remove(path)

    # Convert iconset to icns
    print("Converting to .icns...")
    result = subprocess.run(["iconutil", "-c", "icns", iconset_dir], capture_output=True)

    if result.returncode == 0:
        # Move to app bundle
        icns_path = "ParticleSystem.icns"
        dest_path = "ParticleSystem.app/Contents/Resources/ParticleSystem.icns"
        if os.path.exists(icns_path):
            subprocess.run(["mv", icns_path, dest_path])
            print(f"Icon created: {dest_path}")

        # Cleanup iconset
        subprocess.run(["rm", "-rf", iconset_dir])
        print("Done!")
    else:
        print("Error creating icon:", result.stderr.decode())

if __name__ == "__main__":
    main()
