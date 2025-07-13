.PHONY:build
build:
	~/.pico-sdk/ninja/v1.12.1/ninja -C ./build

.PHONY:flash
flash: build
	~/.pico-sdk/picotool/2.1.1/picotool/picotool load ./build/led_display.elf -fx
