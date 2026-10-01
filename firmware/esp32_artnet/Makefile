CXX ?= g++
NODE ?= node
PROCESSING_JAVA ?= /mnt/c/Program Files (x86)/processing-3.5.4/processing-java.exe
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Werror -O2 -Ifirmware/include
FIRMWARE_SRCS := $(wildcard firmware/src/*.cpp)
PROCESSING_SKETCH := C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\processing\ArtNetMixedLedVisualizer
PROCESSING_OUTPUT := C:\Users\jimmy\Documents\~ pROJECTs ~\Schrank-LED\build\processing-visualizer-p3

.PHONY: all test firmware-build check clean benchmark processing-build processing-run visualizer-test-esp32 output0-matrix16-animation fastled-sketch-player fastled-sketch-player-loop fastled-sketch-player-start fastled-sketch-player-stop

all: build/test_firmware_core

build:
	mkdir -p build reports

firmware/include/WebUi.generated.h: web/index.html web/app.js web/styles.css tools/build-web-ui.js
	$(NODE) tools/build-web-ui.js

build/test_firmware_core: $(FIRMWARE_SRCS) $(wildcard firmware/include/*.h) tests/test_firmware_core.cpp firmware/include/WebUi.generated.h | build
	$(CXX) $(CXXFLAGS) $(FIRMWARE_SRCS) tests/test_firmware_core.cpp -o $@

build/test_network_boot_guard: firmware/src/NetworkManager.cpp firmware/include/NetworkManager.h tests/test_network_boot_guard.cpp $(wildcard tests/stubs/network/*.h) | build
	$(CXX) -Itests/stubs/network $(CXXFLAGS) -DARDUINO firmware/src/NetworkManager.cpp tests/test_network_boot_guard.cpp -o $@

build/test_flexible_output_profile: firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp tests/test_flexible_output_profile.cpp $(wildcard firmware/include/*.h) | build
	$(CXX) $(CXXFLAGS) -DLED_PROFILE_FLEX8 firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp tests/test_flexible_output_profile.cpp -o $@

build/test_extension_board_profile: firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp tests/test_extension_board_profile.cpp $(wildcard firmware/include/*.h) | build
	$(CXX) $(CXXFLAGS) -DLED_PROFILE_FLEX8 -DLED_PROFILE_EXTENSION_BOARD firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp tests/test_extension_board_profile.cpp -o $@

build/test_esp251_profile: firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp firmware/src/UniverseAssembler.cpp tests/test_esp251_profile.cpp $(wildcard firmware/include/*.h) | build
	$(CXX) $(CXXFLAGS) -DLED_PROFILE_FLEX8 -DLED_PROFILE_EXTENSION_BOARD -DLED_PROFILE_ESP251 firmware/src/ConfigManager.cpp firmware/src/OutputRouter.cpp firmware/src/Performance.cpp firmware/src/LogicalPixelBuffer.cpp firmware/src/UniverseAssembler.cpp tests/test_esp251_profile.cpp -o $@

test: build/test_firmware_core build/test_network_boot_guard build/test_flexible_output_profile build/test_extension_board_profile build/test_esp251_profile
	./build/test_firmware_core
	./build/test_network_boot_guard
	./build/test_flexible_output_profile
	./build/test_extension_board_profile
	./build/test_esp251_profile
	$(NODE) tests/test_mapping_schema.js
	$(NODE) tests/test_web_calculations.js
	$(NODE) tests/test_processing_protocol.js
	$(NODE) tests/test_processing_visualizer.js
	$(NODE) tests/test_hardware_benchmark_tool.js
	$(NODE) tests/test_esp_web_menu_playwright.js
	$(NODE) tests/test_web_ui_playwright.js

firmware-build:
	.venv-platformio/Scripts/pio.exe run -e esp32-wifi-ws2812-apa102-1679

check: test firmware-build

benchmark:
	$(NODE) tools/artnet-test-tool.js --scenario all --duration 1 --report reports/benchmark-smoke
	$(NODE) tools/performance-matrix.js

processing-build:
	"$(PROCESSING_JAVA)" --sketch="$(PROCESSING_SKETCH)" --output="$(PROCESSING_OUTPUT)" --force --build

processing-run:
	"$(PROCESSING_JAVA)" --sketch="$(PROCESSING_SKETCH)" --output="$(PROCESSING_OUTPUT)-run" --force --run

visualizer-test-esp32:
	$(NODE) tools/visualizer-test-patterns.js --host 10.0.0.246

output0-matrix16-animation:
	$(NODE) tools/output0-matrix16-animation.js --host 10.0.0.246 --fps 30 --gap 0 --universes 2

fastled-sketch-player:
	$(NODE) tools/fastled-sketch-player.js --host 10.0.0.246 --mode fit16 --fps 60 --brightness 1.0 --speed 0.35 --seconds 300 --pattern-seconds 75 --crossfade 18

fastled-sketch-player-loop:
	$(NODE) tools/fastled-sketch-player.js --host 10.0.0.246 --mode fit16 --fps 60 --brightness 1.0 --speed 0.35 --loop --pattern-seconds 75 --crossfade 18

fastled-sketch-player-start: build
	@if [ -f reports/fastled-sketch-player.pid ] && kill -0 $$(cat reports/fastled-sketch-player.pid) 2>/dev/null; then \
		echo "FastLED sketch player already running pid=$$(cat reports/fastled-sketch-player.pid)"; \
	else \
		nohup node tools/fastled-sketch-player.js --host 10.0.0.246 --mode fit16 --fps 60 --brightness 1.0 --speed 0.35 --loop --pattern-seconds 75 --crossfade 18 > reports/fastled-sketch-player.log 2>&1 & \
		echo $$! > reports/fastled-sketch-player.pid; \
		echo "FastLED sketch player started pid=$$(cat reports/fastled-sketch-player.pid) log=reports/fastled-sketch-player.log"; \
	fi

fastled-sketch-player-stop:
	@if [ -f reports/fastled-sketch-player.pid ] && kill -0 $$(cat reports/fastled-sketch-player.pid) 2>/dev/null; then \
		kill $$(cat reports/fastled-sketch-player.pid); \
		echo "FastLED sketch player stopped pid=$$(cat reports/fastled-sketch-player.pid)"; \
		rm -f reports/fastled-sketch-player.pid; \
	else \
		pkill -f "[t]ools/fastled-sketch-player.js" 2>/dev/null && echo "Stopped matching FastLED sketch player processes" || echo "FastLED sketch player is not running"; \
		rm -f reports/fastled-sketch-player.pid; \
	fi

clean:
	rm -rf build reports/*.json reports/*.csv
