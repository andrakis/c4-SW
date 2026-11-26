# c4-images.sh c4ix|bare -- Shadow Warrior's part of c4's build-images.sh,
# which sources it from c4's root (games/README.md in c4)
GAMEDIR=games/sw
case "$1" in
c4ix)
    # linked against C4IX's libc, with the shareware episode beside it
    # (c4ix.vfs.txt: /usr/games/sw). C4IX's disk is flat and shared, so
    # the package's licence goes there under a name of its own.
    if [ -f .sw/sw-sum.c4o ] && [ -f $PB/lib/libc.c4s ]; then
        say "linking Shadow Warrior for C4IX"
        ./c4rlink32 .sw/*.c4o .sw/plat/sw-c4ix.c4o -l $PB/lib/libc.c4s -o $DISK/c4ix-sw.c4r > /dev/null
        if [ -f $GAMEDIR/data/SWSW.GRP ]; then
            cp $GAMEDIR/data/SWSW.GRP $GAMEDIR/data/SW.RTS $DISK/
            cp $GAMEDIR/data/LICENSE.TXT $DISK/sw-license.txt
        fi
        # the Build engine's licence travels with the engine (BUILDLIC.TXT)
        cp $GAMEDIR/jfbuild/buildlic.txt $DISK/sw-buildlic.txt
    fi ;;
bare)
    if [ -f .sw/sw-sum.c4o ]; then
        item sw
        ./c4rlink32 .sw/*.c4o .sw/plat/sw-bare.c4o .libc-bare/libc-*.c4o -o $GAMESDISK/sw.c4r > /dev/null
        # the games disk is flat, and Duke Nukem 3D's BUILDLIC.TXT is there
        cp $GAMEDIR/jfbuild/buildlic.txt $GAMESDISK/SWBUILD.TXT
        if [ -f $GAMEDIR/data/SWSW.GRP ]; then
            cp $GAMEDIR/data/SWSW.GRP $GAMEDIR/data/SW.RTS $GAMESDISK/
            cp $GAMEDIR/data/LICENSE.TXT $GAMESDISK/SWLIC.TXT
        fi
        GAMES_ON_DISK="$GAMES_ON_DISK SW"
    fi ;;
esac
