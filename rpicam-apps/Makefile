.PHONY: all clean setup compile install test coverage clean_images

all: clean setup compile install test coverage

clean: clean_images
	rm -rf build

setup:
	# Set up the build with the 'debug' build type for coverage
	meson setup build --buildtype=debug -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=disabled -Denable_tflite=disabled

compile:
	meson compile -C build

install:
	sudo meson install -C build
	sudo ldconfig

test:
	# Run the Python tests located in rpicam-apps/utils/
	mkdir -p test_output

	python3 utils/test.py --exe-dir ./build/ --output-dir ./test_output --json-dir ./assets/

coverage:
	# Generate coverage report (adjust for your coverage tool if needed, e.g., lcov or gcov)
	lcov --directory build --capture --output-file coverage.info
	lcov --remove coverage.info '/usr/*' --output-file coverage_filtered.info
	genhtml coverage_filtered.info --output-directory coverage_report
	@echo "Coverage report generated at coverage_report/index.html"

clean_images:
	rm -f *.jpg
