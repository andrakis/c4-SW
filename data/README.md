# Shadow Warrior's data

`3dsw12.zip` is 3D Realms' shareware package of Shadow Warrior v1.2,
13,299,147 bytes, unmodified, as 3D Realms' FTP site had it. It holds the
first episode, "Enter the Wang".

Its `LICENSE.TXT` (V.5.01.97) lets individuals share copies and give them
to friends, family, coworkers and members of any not-for-profit
organization, only without charge, as long as no copyright information or
trademark is added or removed and the game's files are included without
modification. The registered game, and any modified version, may not be
distributed: the full game is the person's own `SW.GRP`, which they add
with the page's Uploads (README.c4.md). The licence names no web or FTP
sites, as Duke Nukem 3D's later one does; c4's page is free, and its
published site carries the episode, as it carries Duke's (c4's
`libjs/pages.sh`).

`tools/swdata.py` reads the game's files out of the package and puts them
here, and the disks get them:

- `SWSW.GRP`, the package's `SW.GRP` (26,056,769 bytes, CRC32
  `08A7FA1F`, JFShadowWarrior's "Shareware Version"), renamed so that the
  person's own `SW.GRP` never hides it on a disk the Uploads are laid over;
- `SW.RTS`, the remote ridicule sounds;
- `LICENSE.TXT`, the licence itself.

The package holds them in `SWSW12.SHR`, a zip by another name.

## Getting it

None of these files is in this repository. In c4, `make sw-data` puts
them here with c4's `games/fetch.sh`:
- from `$SW_ZIP`, when that names your copy;
- or else from the first URL in `MIRRORS` that gives exactly this file.

Either way it is refused unless its SHA-256 is
`6e41b3769d0373fc160c2113469c6f8bc2932dc1a77b69c81c1bd9644a189a8f`.
The URL in `MIRRORS` was checked to give exactly that file (2026-11-26).
