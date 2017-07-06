NAME=tubeguy
SOUNDS = clang splat explosion tada welldone

GAME_C_FILES=$(NAME).c loop_song.c lib/blitter/blitter.c lib/sampler/sampler.c lib/blitter/blitter_tmap.c lib/blitter/blitter_sprites.c
GAME_BINARY_FILES = build/title.spr build/screen3.tmap build/screen3.tset build/levels.tmap \
    build/cursor.spr build/logo.spr build/splash.spr build/bonh.spr \
    $(SOUNDS:%=build/%.snd)
GAME_C_OPTS = -DVGA_MODE=400


include $(BITBOX)/kernel/bitbox.mk

$(NAME).c: build/screen3.h build/levels.h build/binaries.h tubes.h

tubes.h: tubes.tmx mk_tubes.py
	python mk_tubes.py > $@

build/%.h build/%.tmap build/%.tset: %.tmx tiles3.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/tmx.py -o $(dir $@) $< > $@

build/title.spr: title.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/couples_encode2.py $<
	mv title.spr $@

build/logo.spr: logo2_4col.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/couples_encode2.py $<
	mv logo2_4col.spr $@

build/splash.spr: splash.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/sprite_encode_rle.py $@ $^

build/cursor.spr: cursor0.png cursor1.png cursor2.png cursor3.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/sprite_encode2.py -m p4 $@ $^


build/bonh.spr: guy_*.png
	@mkdir -p $(dir $@)
	python $(BITBOX)/lib/blitter/scripts/sprite_encode_rle.py $@ $^

loop_song.c: sounds/loop.mid
	python $(BITBOX)/lib/sampler/sampler_read_midi.py $< > $@

build/%.snd: sounds/%.wav
	sox $< -t s8 -r 11025 $@

clean:: 
	rm -f loop_song.c 