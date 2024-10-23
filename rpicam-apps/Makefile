.PHONY: all clean setup compile install

all: clean setup compile install

clean:
	rm -rf build

setup:
	meson setup build -Denable_libav=enabled -Denable_drm=enabled -Denable_egl=enabled -Denable_qt=enabled -Denable_opencv=enabled -Denable_tflite=disabled

compile:
	meson compile -C build

install:
	sudo meson install -C build
	sudo ldconfig