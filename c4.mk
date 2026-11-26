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
SW_C4     := sw-layer sw-sum sw-gui sw-snd
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
.sw/%.c4o: $(SW)/jfbuild/src/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
.sw/%.c4o: $(SW)/jfmact/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
.sw/%.c4o: $(SW)/src/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
.sw/%.c4o: $(SW)/c4/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
.sw/%.c4o: $(SW)/c4/audiolib/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
	@mkdir -p .sw/plat
	$(SW_CC) $< $@ > /dev/null
.sw/plat/%.c4o: $(SW)/c4/%.c $(SW_HDRS) $(C4LC_LISP) .img_g | $(LC32_BIN)
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
