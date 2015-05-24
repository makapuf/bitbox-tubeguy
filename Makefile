NAME=tubeguy
GAME_C_FILES=$(NAME).c 
GAME_BINARY_FILES = build/title.spr build/screen3.tmap build/screen3.tset build/tubes3.tmap build/cursor.spr
GAME_C_OPTS = -DVGAMODE_400
USE_ENGINE=1

include $(BITBOX)/lib/bitbox.mk

$(NAME).c: build/screen3.h build/tubes3.h
build/%.h build/%.tmap build/%.tset: %.tmx
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/tmx.py -o $(dir $@) $< > $@

build/title.spr: title.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode_rle.py $@ $< 

build/cursor.spr: cursor*.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode2.py -m p4 $@ $<