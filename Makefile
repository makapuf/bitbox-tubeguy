NAME=tubeguy
GAME_C_FILES=$(NAME).c 
GAME_BINARY_FILES = build/title.spr build/screen3.tmap build/screen3.tset build/tubes3.tmap build/cursor.spr build/logo.spr build/splash.spr build/bonh.spr
GAME_C_OPTS = -DVGAMODE_400
USE_ENGINE=1

include $(BITBOX)/lib/bitbox.mk

$(NAME).c: build/screen3.h build/tubes3.h
build/%.h build/%.tmap build/%.tset: %.tmx tiles3.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/tmx.py -o $(dir $@) $< > $@

build/title.spr: title.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode_rle.py $@ $< 

build/logo.spr: logo2_4col.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode_rle.py $@ $< 

build/cursor.spr: cursor*.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode2.py -m p4 $@ $<

build/splash.spr: splash.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/scripts/sprite_encode_rle.py $@ $<

build/bonh.spr: guy_*.png
	python $(BITBOX)/scripts/sprite_encode_rle.py $@ $^
	#python $(BITBOX)/scripts/sprite_encode2.py -m p4 $@ $^
	
