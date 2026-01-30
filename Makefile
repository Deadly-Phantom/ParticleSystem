# Particle System Makefile

APP_NAME = ParticleSystem
EXECUTABLE = particles
SOURCE = particles.cpp

# Compiler settings
CXX = g++
CXXFLAGS = $(shell sdl2-config --cflags)
LDFLAGS = $(shell sdl2-config --libs)

# App bundle paths
APP_BUNDLE = $(APP_NAME).app
APP_CONTENTS = $(APP_BUNDLE)/Contents
APP_MACOS = $(APP_CONTENTS)/MacOS
APP_RESOURCES = $(APP_CONTENTS)/Resources

# Default target
all: app

# Build executable
$(EXECUTABLE): $(SOURCE)
	$(CXX) -o $@ $< $(CXXFLAGS) $(LDFLAGS)

# Build app bundle
app: $(EXECUTABLE)
	@mkdir -p $(APP_MACOS) $(APP_RESOURCES)
	@cp $(EXECUTABLE) $(APP_MACOS)/$(APP_NAME)
	@if [ ! -f $(APP_RESOURCES)/$(APP_NAME).icns ]; then \
		echo "Generating icon..."; \
		python3 generate_icon.py; \
	fi
	@codesign --force --deep --sign - $(APP_BUNDLE) 2>/dev/null || true
	@touch $(APP_BUNDLE)
	@echo "Built: $(APP_BUNDLE)"

# Build and run
run: app
	@open $(APP_BUNDLE)

# Run executable directly (faster, no app bundle)
quick:
	$(CXX) -o $(EXECUTABLE) $(SOURCE) $(CXXFLAGS) $(LDFLAGS) && ./$(EXECUTABLE)

# Clean build artifacts
clean:
	rm -f $(EXECUTABLE)

# Clean everything including app bundle
cleanall: clean
	rm -rf $(APP_BUNDLE) $(APP_NAME).iconset

# Regenerate icon
icon:
	python3 generate_icon.py

.PHONY: all app run quick clean cleanall icon
