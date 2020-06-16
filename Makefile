SKETCH := feather_geiger_influx
BOARD := esp8266:esp8266:huzzah
PORT := /dev/ttyUSB0

.PHONY: all
all: $(SKETCH).bin

.PHONY: install
install: $(SKETCH).bin
	arduino-cli upload -p $(PORT) -b $(BOARD) -i $<

.PHONY: clean
clean:
	rm -f $(SKETCH).bin

$(SKETCH).bin: $(SKETCH).ino
	arduino-cli compile -b $(BOARD) -o $@ $<
	rm -f $(SKETCH).elf
