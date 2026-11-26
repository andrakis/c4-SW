# games/sw — Shadow Warrior on C4IX, C4DOS and the bare machine

3D Realms' Shadow Warrior (1997), as JFShadowWarrior has it (Jonathon
Fowler's port of the GPL source 3D Realms released in 2005, on his port of
Ken Silverman's Build engine, JFBuild), compiled by c4lc and played on
libjs's whole display, with its sound effects on libjs's sound card and
its music on the card's OPL, or, when the person chooses General MIDI
music, as General MIDI on the card's MIDI port. The shareware episode,
Enter the Wang, ships with it; the full game plays from the person's own
`SW.GRP`. It is played from the keyboard and the mouse.

This repository is JFShadowWarrior's history (`upstream`,
github.com/jonof/jfsw) with c4's port on top as commits, on branch `c4`.
jfsw's three submodules, JFBuild, jfaudiolib and JFMact, are copied in as
directories at the commits jfsw names (the branch's first commit says
which), so that c4 takes the game as one repository.

## Where it runs

| where | how | platform |
|---|---|---|
| C4IX, the whole display | `sw`, or Go > Games > Shadow Warrior; the desktop gives way until it ends | `sw-gui.c`, `sw-c4ix.c` |
| C4IX, a window | `sw -window` in a Terminal; also when a program that does not give way has the display | `sw-gui.c`, `sw-c4ix.c` |
| C4DOS | `\GAMES\SW\SW` at the prompt | `sw-gui.c`, `sw-bare.c` |
| grin, under C4DOS | Program Manager's Action Games > Shadow Warrior, full screen | `sw-gui.c`, `sw-bare.c` |
| the bare machine | `sw.c4r` as the machine's image; the page's `?system=sw` | `sw-gui.c`, `sw-bare.c` |

Keys are the game's own, and its menus (Options > Input Menu) change
them. The mouse locks to the display at the first click, until Esc, and
turns and walks as the game's mouse did.

## The full game: your own files

`sw -home` (Go > Games > Shadow Warrior (your files); Program Manager's
Full Games (Uploads) > Shadow Warrior under C4DOS; the page's
`?system=sw-full` on the bare machine) plays your own `SW.GRP` from
Shadow Warrior 1.2, the registered game. Add it with the page's Uploads,
in the folder `/home/sw` (one uploaded to `/home` itself plays too), then
start the game. It looks in its own folder first, then in `/home`
(`sw_find_grp`, `c4/sw-sum.c`): on C4IX `/home/sw/sw.grp`, then
`/home/SW.GRP`; on the bare machine `sw/sw.grp`, then `SW.GRP`; under
C4DOS `B:\SW\SW.GRP`, then `B:\SW.GRP`.

The shareware episode is `SWSW.GRP` on c4's disks, so that your `SW.GRP`
never hides it: `sw` alone always plays the shareware. Without your file,
`sw -home` says what it wants and stops. The GRP is known by its size
(JFShadowWarrior's table, `src/grpscan.c`), not its CRC, which would read
all of it at every start: the shareware 1.2 and the registered 1.2. The
add-ons (Wanton Destruction, Twin Dragon) are not looked for. The
registered game's CD music is not played: c4 has no CD.

## Settings, and the screen size

Settings are changed in the game, in Options (Esc), and kept in `sw.cfg`
where the game runs. **The screen size** is Options > Screen Menu >
Resolution: Left and Right choose, Apply Settings takes it. The sizes are
320x200 (the default, as the DOS game ran), 320x240, 400x300, 512x384,
640x400, 640x480 and 800x600. The engine draws at the size chosen and
the display scales it to fill the screen; the status bar and the menus
are drawn bigger. The time a frame takes grows with its pixels.

## What is c4's here

Everything in `c4/`, and a few lines in jfsw's and JFBuild's own files,
each under `#ifdef C4SW`:

- `c4/sw-layer.c`, in place of JFBuild's SDL layer (`jfbuild/src/sdlayer2.c`),
  which it follows function by function (`baselayer.h`): `main`, the
  keys as PC scan codes with the characters they type, the mouse, Ken's
  timer on the game's clock, the mode list, and each frame with its
  palette. A palette is shown as soon as it is set: the game fades by the
  palette alone.
- `c4/sw-gui.c`, `sw-c4ix.c`, `sw-bare.c`, `sw-host.c`: the platforms, as
  Duke Nukem 3D's (c4's `games/duke3d`). Frames go out 8-bit, as
  `GUI_BLIT8` with their palette, scaled to a 4:3 display at least 640
  wide.
- `c4/sw-snd.c`, in place of jfaudiolib: its `FX_`, `MUSIC_` and `CD_`
  calls on libjs's sound card, from Duke Nukem 3D's `duke-snd.c`. Effects
  on the card's sixteen sample channels; with all of them playing, a new
  sound takes the oldest of the lowest priority (one panned out of hearing
  first), as audiolib did. Music by audiolib's MIDI player and AdLib
  driver (`c4/audiolib`, from c4's Duke Nukem 3D port) on the card's OPL,
  with the game's own timbre bank (`SWTIMBR.TMB`), or as General MIDI on
  the card's MIDI port. No card: no sound, as if None were chosen.
  `c4/sw-nosnd.c` is the silent one, for gcc's build.
- `c4/sw-sum.c`: the clock, one `rand` for every build, the GRP, the
  test's input and every frame checksummed.
- `c4/inc/`: the headers C4IX's libc lacks (`stdint.h`, `stdbool.h`,
  `inttypes.h`), and DOS's for audiolib (`conio.h`, `dos.h`).
- `c4.mk`, `c4-images.sh`, `c4ix.vfs.txt`, `tools/swdata.py`: c4's build,
  its disks, the desktop's Games menu, and the data (`data/README.md`).

In jfsw's and JFBuild's files:

- `jfbuild/include/compat.h`: c4lc is taken as C99 enough, the machine as
  little-endian, and what C4IX's libc does not name (`ptrdiff_t`,
  `S_IFLNK`, `S_IRWXU`). `c4/inc/stdint.h`'s `SIZE_MAX` is `INT_MAX`:
  C4IX's `size_t` is an `int`, and an unsigned `SIZE_MAX` was -1 there,
  so JFBuild's cache took every place as full.
- `jfbuild/src/compat.c`, `kplib.c`: `stat` by path, for C4IX has no
  `fstatat`.
- `src/grpscan.c`, `game.c`: the GRP is `sw-sum.c`'s to find, and known
  by its size; the settings and saved games where the game is, not in a
  profile folder.
- `src/game.c`: c4's switches (`-sum`, `-script`, `-home`, ...) are
  passed over by the game's, which take `-s...` for a skill and `-h...`
  for `-h`; `RAND_MAX` is 32767, as `rand` is in every build (glibc's
  2147483647 made the screen shakes other than c4lc's); the art's cache
  is 12 MB, as Duke's, where jfsw asks 32; a quit goes straight back to
  the desktop or the prompt, without the shareware's ordering screen,
  which waited for a key in a loop that never slept.
- `src/sounds.c`: with no sound card the sound is off, as when None is
  chosen. Left on, each intermittent ambient sound that could not start
  queued itself again in the same pass until the memory ran out
  (jfaudiolib's own null driver "plays" them silently).
- `src/config.c`: 320x200 by default; a test reads and writes no
  `sw.cfg`.
- `src/version.c`: no `__DATE__` or `__TIME__`, which c4lc has not (and
  c4's images are the same bytes each time they are built).
- `src/draw.c`, `game.c`: `-trace` prints where the player is and what
  is drawn from, to tell two builds apart.

c4lc took the rest as it is, once it took five things C does and it did
not (c4's commit "c4lc: what Shadow Warrior's sources need"): a backslash
before a CR LF (jfaudiolib's files have DOS line ends), several names in
one typedef, typedefs of function types (`typedef int ANIMATOR(short);`
then `ANIMATOR DoBunnyMove, NullBunny;`, some 250 lines), a static
local's initializer naming static locals, and an array's size after a
block's `extern T a[];`.

## Testing

- `make sw-data` takes 3D Realms' `3dsw12.zip` from `$SW_ZIP` or
  `data/MIRRORS`, by its SHA-256, and reads the game's files out of it
  (`data/README.md`). They are never committed.
- `make test-sw` plays the first level (`-level1 -s2`) from a fixed
  script of keys, mouse moves and a mouse button, on a clock that moves
  only as the game runs, built by gcc and by c4lc, on the bare machine,
  under C4DOS and inside C4IX: all must show the same frames, to the
  checksum of every pixel and the palette. Then, inside C4IX, the screen
  size changed in the game's menu (Options, Screen Menu, Resolution,
  Apply Settings) from 320x200 to 400x300, a walk at that size, and a
  quit from the game's menu (Quit, Y), which must end it. And on the bare
  machine with the sound and the music on, on a General MIDI card: the
  music must play, drums and all. The shareware has no demos to play.
- The test's clock moves a millisecond each time the game looks for its
  input, and a frame's time with each frame, not when the engine samples
  the timer while it draws (`faketimerhandler`, every 16 columns of a
  wall): how often that happens is the renderer's business, and it
  differed between gcc's build and c4lc's.
- `sw -sum N -frames N -script -shot N -trace` are the test's: a checksum
  line every N frames, a stop, the script (`-menu` for the menu's), every
  Nth frame as a PPM (with `-trace`, its rows, columns and pixels on
  stdout, for the bare machine), and the player's place each frame.
