# c4.mk -- this game's part of c4's Makefile. c4 includes it from
# games/sw/c4.mk, its submodule, and runs it from c4's root, with c4's
# tools and variables (games/README.md in c4). Without it, c4 builds
# everything else, and test-sw says it was skipped.

# Shadow Warrior (games/sw/README.c4.md): JFShadowWarrior's units with
# c4lc -- the game, JFBuild's engine and JFMact -- the same objects for
# C4IX, the bare machine and C4DOS, but for the two that say whether the
# display has to be asked for (sw-c4ix.c, sw-bare.c, in .sw/plat).
# c4-images.sh links them: against C4IX's libc, and whole with the bare
# libc of libc-bare-objs. jfaudiolib is not built: sw-snd.c plays the
# game's sounds on the card, and Duke Nukem 3D's audiolib (c4/audiolib)
# its music on the card's OPL.
SW        := games/sw
SW_ENG    := a-c baselayer cache1d compat crc32 defs engine kplib mmulti_null osd pragmas scriptfile \
             textfont talltextfont smalltextfont startwin_stub
SW_MACT   := util_lib file_lib control keyboard mouse mathutil scriplib animlib
SW_GAME   := actor ai anim border break bunny cache cheats colormap config console coolg coolie copysect \
             demo draw eel game girlninj goro grpscan hornet interp interpsh inv jplayer jsector jweapon \
             lava light mclip menus miscactr morph net ninja osdcmds osdfuncs panel player predict quake \
             ripper ripper2 rooms rotator rts save scrip2 sector serp setup skel skull slidor sounds spike \
             sprite sumo swconfig sync text track vator vis wallmove warp weapon zilla zombie saveable version
# c4's alone: the platform layer, the clock and the test, the display, the card
SW_C4     := sw-layer sw-sum sw-gui sw-snd sw-math
# audiolib's MIDI player and AdLib driver, for the music on the card's OPL
SW_AUD    := midi al_midi gmtimbre ll_man usrhooks
SW_PLAT   := sw-c4ix sw-bare
SW_HDRS   := $(wildcard $(SW)/src/*.h $(SW)/jfbuild/include/*.h $(SW)/jfbuild/src/*.h $(SW)/jfmact/*.h \
             $(SW)/jfaudiolib/include/*.h $(SW)/c4/*.h $(SW)/c4/inc/*.h $(SW)/c4/audiolib/*.h) \
             $(wildcard src/c4ix/libc/include/*.h src/c4ix/libc/include/sys/*.h) \
             src/c4ix/include/display.h src/c4ix/include/window.h src/c4ix/include/window.c \
             libjs/guest/gui.h libjs/guest/gui.c libjs/guest/snd.h libjs/guest/snd.c libjs/guest/accel.h
SW_INC    := -D C4SW -D rand=sw_rand -D srand=sw_srand -I $(SW)/c4/inc -I src/c4ix/libc/include \
             -I src/c4ix/include -I libjs/guest -I $(SW)/src -I $(SW)/jfbuild/include -I $(SW)/jfbuild/src \
             -I $(SW)/jfmact -I $(SW)/jfaudiolib/include -I $(SW)/c4
SW_CC     := $(call lc32,-c 400000) -O $(IMG_G) -mfuse -fno-builtin -c -conforming $(SW_INC)
SW_OBJS   := $(SW_ENG:%=.sw/%.c4o) $(SW_MACT:%=.sw/%.c4o) $(SW_GAME:%=.sw/%.c4o) $(SW_C4:%=.sw/%.c4o) \
             $(SW_AUD:%=.sw/%.c4o)
sw-objs: $(SW_OBJS) $(SW_PLAT:%=.sw/plat/%.c4o)
# each list from its own directory: the game and JFBuild both have a
# config.c and a version.c (JFBuild's are its editor's), and a pattern
# rule would take whichever it met first
$(SW_ENG:%=.sw/%.c4o): .sw/%.c4o: $(SW)/jfbuild/src/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
$(SW_MACT:%=.sw/%.c4o): .sw/%.c4o: $(SW)/jfmact/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
$(SW_GAME:%=.sw/%.c4o): .sw/%.c4o: $(SW)/src/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
$(SW_C4:%=.sw/%.c4o): .sw/%.c4o: $(SW)/c4/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
$(SW_AUD:%=.sw/%.c4o): .sw/%.c4o: $(SW)/c4/audiolib/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
$(SW_PLAT:%=.sw/plat/%.c4o): .sw/plat/%.c4o: $(SW)/c4/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
PHONY += sw-objs

# ... and by gcc for the host, 32-bit as the c4lc build is, with no display
# and no sound (sw-host.c, sw-nosnd.c): the reference test-sw holds the
# c4lc builds to.
SW_HOST_SRCS := $(SW_ENG:%=$(SW)/jfbuild/src/%.c) $(SW_MACT:%=$(SW)/jfmact/%.c) $(SW_GAME:%=$(SW)/src/%.c) \
                $(SW)/c4/sw-layer.c $(SW)/c4/sw-sum.c $(SW)/c4/sw-host.c $(SW)/c4/sw-nosnd.c
sw-host: $(SW_HOST_SRCS) $(SW_HDRS)
	gcc -m32 -std=gnu99 -O2 -msse2 -mfpmath=sse -ffp-contract=off -fno-strict-aliasing -w \
	    -DC4SW -DC4SW_HOST -Drand=sw_rand -Dsrand=sw_srand -I$(SW)/c4 -I$(SW)/src -I$(SW)/jfbuild/include \
	    -I$(SW)/jfbuild/src -I$(SW)/jfmact -I$(SW)/jfaudiolib/include -Ilibjs/guest \
	    -o $@ $(SW_HOST_SRCS) -lm

# The data, which is not in the repository: 3D Realms' shareware package
# of v1.2 as it was distributed, checked by its SHA-256, and what the game
# reads out of it (tools/swdata.py): the episode's SW.GRP as SWSW.GRP, so
# that the person's own never hides it, SW.RTS, and the package's
# LICENSE.TXT (data/README.md). games/fetch.sh takes the package from
# $SW_ZIP when that names a copy, or else from the mirrors in data/MIRRORS.
SW_DATA := $(SW)/data
sw-data: $(SW_DATA)/SWSW.GRP
$(SW_DATA)/3dsw12.zip:
	@sh games/fetch.sh $@ 6e41b3769d0373fc160c2113469c6f8bc2932dc1a77b69c81c1bd9644a189a8f SW_ZIP $$(cat $(SW_DATA)/MIRRORS 2>/dev/null)
$(SW_DATA)/SWSW.GRP: $(SW_DATA)/3dsw12.zip $(SW)/tools/swdata.py
	@python3 -I $(SW)/tools/swdata.py $< $(SW_DATA)
PHONY += sw-data

# test-sw: the first level (-level1, skill 2) played from a fixed script
# of keys, mouse moves and a mouse button (sw-sum.c, -script), on a clock
# that moves only as the game runs, by gcc's build and by c4lc's -- on the
# bare machine, under C4DOS and inside C4IX -- must show the same frames,
# to the checksum of every pixel and the palette. With the sound off
# (-nosound): gcc's build has no card. The bare machine's and C4DOS's with
# the accelerators, which must draw what the loops do (and acceltest
# holds each to its loop first). Then, inside C4IX, the screen size
# changed as a player changes it, in the game's menu (Options, Screen
# Menu, Resolution, Apply Settings: -menu, from 320x200 to 400x300), and
# played on so; then quit from the game's menu (Quit, Y), which must end
# it. And on the bare machine once more with the sound and the music on,
# on a sound card with a General MIDI synth (cli.js --midi, sw-snd.c):
# the music as MIDI, drums and all (midi-notes.mjs). Its frames are not
# the silent runs': the game's ambient sounds draw on its random numbers
# when they can play. Without the accelerators: the music's time is the
# machine's, and the notes counted are those the 600 frames take.
SW_RUN := -level1 -s2 -nosound -sum 50 -frames 600 -script
SW_RES := -level1 -s2 -nosound -sum 20 -frames 500 -menu
SW_GM  := -level1 -s2 -sum 50 -frames 600 -script
# c4's port: Shadow Warrior's accelerators (c4/accel.mjs) against their
# loops, on the bare machine, with libjs's own (cli.js loads them,
# docs/accel.md). In a directory of its own: c4-images.sh links
# .sw/*.c4o into the game.
.sw/test/acceltest.c4o: $(SW)/c4/acceltest.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/test
	$(SW_CC) $< $@ > /dev/null
.sw/test/acceltest.c4r: .sw/test/acceltest.c4o libc-bare-objs
	./c4rlink32 $< .libc-bare/libc-*.c4o -o $@ > /dev/null
test-sw: sw-data sw-host c4bb-images test-libc-bare .sw/test/acceltest.c4r
	@rm -rf .sw_t && mkdir -p .sw_t
	@timeout 600 node libjs/cli.js -m 64 .sw/test/acceltest.c4r > .sw_t/accel.txt 2>&1; cat .sw_t/accel.txt
	@grep -q "^accel: Shadow Warrior's, [0-9]* cases, each as its loop has it" .sw_t/accel.txt \
	   || { echo "test-sw: the accelerators do not draw as their loops"; exit 1; }
	@cp $(SW_DATA)/SWSW.GRP .sw_t/
	@cd .sw_t && ../sw-host $(SW_RUN) 2>/dev/null | grep '^sum\|^timedemo' > host.txt
	@cd .sw_t && ../sw-host $(SW_RES) 2>/dev/null | grep '^sum\|^timedemo' > res.txt
	@tail -1 .sw_t/host.txt
	@cd src/c4bb/images/games && { timeout 1800 node ../../../../libjs/cli.js -m 128 -d . sw.c4r $(SW_RUN) \
	     > ../../../../.sw_t/bare.out 2>/dev/null & \
	   printf 'sw $(SW_RUN)\nver\n' | timeout 1800 node ../../../../libjs/cli.js -m 128 -d . ../c4dos32.c4r \
	     > ../../../../.sw_t/dos.out 2>/dev/null; wait; }
	@printf 'sw $(SW_RUN)\nexit\n' | timeout 1800 node libjs/cli.js -m 128 -d src/c4bb/images/disk src/c4bb/images/c4ix32.c4r \
	   2>/dev/null > .sw_t/c4ix.out
	@{ cd src/c4bb/images/games && timeout 1800 node ../../../../libjs/cli.js -m 128 --no-accel -d . --midi ../../../../.sw_t/gm.txt sw.c4r \
	     $(SW_GM) > ../../../../.sw_t/gm.out 2>/dev/null; } & \
	 printf 'sw $(SW_RES)\nexit\n' | timeout 1800 node libjs/cli.js -m 128 -d src/c4bb/images/disk src/c4bb/images/c4ix32.c4r \
	   2>/dev/null > .sw_t/res.out; wait
	@for w in bare dos c4ix; do grep '^sum\|^timedemo' .sw_t/$$w.out | cmp -s .sw_t/host.txt - \
	   || { echo "test-sw: $$w's frames differ:"; tail -5 .sw_t/$$w.out; exit 1; }; done
	@grep '^sum\|^timedemo' .sw_t/res.out | cmp -s .sw_t/res.txt - \
	   || { echo "test-sw: with the screen size changed in the menu, the frames differ:"; tail -5 .sw_t/res.out; exit 1; }
	@grep -q '^sw: quit from the game' .sw_t/res.out \
	   || { echo "test-sw: the game's quit did not end it:"; tail -5 .sw_t/res.out; exit 1; }
	@grep -q '^timedemo: 600 frames' .sw_t/gm.out \
	   || { echo "test-sw: with the sound and the music on, the game did not play on:"; tail -5 .sw_t/gm.out; exit 1; }
	@m=$$(node libjs/tests/src/midi-notes.mjs .sw_t/gm.txt 1000 4) && echo "test-sw: General MIDI: $$m" \
	   || { echo "test-sw: the General MIDI music did not play: $$m"; exit 1; }
	@grep -A1 '^timedemo' .sw_t/dos.out | grep -Eq '^A(:\\)?>C4DOS version' \
	   || { echo "test-sw: C4DOS did not come back to its prompt:"; tail -5 .sw_t/dos.out; exit 1; }
	@rm -rf .sw_t
	@echo "test-sw: OK"
PHONY += test-sw

GAME_OBJS  += sw-objs
GAME_TESTS += test-sw
