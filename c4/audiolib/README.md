# c4/audiolib

Apogee's Sound System (James R. Dose's audiolib, as 3D Realms released it
under the GPL with Duke Nukem 3D's source): the MIDI player (`midi.c`),
its AdLib driver (`al_midi.c`), the General MIDI timbre bank
(`gmtimbre.c`), the list manager they use (`ll_man.c`) and the memory
hooks (`usrhooks.c`), with their headers. Shadow Warrior's DOS version played its music through these.

They are copied from c4's Duke Nukem 3D port (andrakis/c4-Duke3D, branch
`c4`, commit 15e0c92, `Game/src/audiolib`), with c4's changes for libjs's
sound card there (`C4DUKE`, here `C4SW`): no wait on DOS's clock, the
AdLib's tracks of an extended MIDI song, and an AdLib whatever the card.
`../sw-snd.c` drives them on the card's OPL (or its General MIDI synth).
