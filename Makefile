build:
	mkdir -p bin
	# gcc src/esp_code.c -o bin/espcode -lcurl
	esptool.py erase_flash