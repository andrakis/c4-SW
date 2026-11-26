#!/usr/bin/env python3
# swdata.py ZIP DIR -- what c4's disks take from 3D Realms' shareware
# package of Shadow Warrior v1.2 (3dsw12.zip), into DIR:
#
#   SWSW.GRP     the episode (the package's SW.GRP), named so that the
#                person's own SW.GRP never hides it
#   SW.RTS       the remote ridicule sounds
#   LICENSE.TXT  3D Realms' terms for the shareware
#
# The package holds them in SWSW12.SHR, a zip by another name, which
# Python's zipfile reads as it is. Nothing is changed; each file is
# checked against its size, and the GRP against its CRC, which is
# JFShadowWarrior's "Shareware Version" (src/grpscan.c).
import sys, os, io, zipfile, zlib

z, out = sys.argv[1], sys.argv[2]
outer = zipfile.ZipFile(z)
inner = zipfile.ZipFile(io.BytesIO(outer.read('SWSW12.SHR')))
want = {'SW.GRP': ('SWSW.GRP', 26056769), 'SW.RTS': ('SW.RTS', 267626),
        'LICENSE.TXT': ('LICENSE.TXT', 6689)}
for name, (dest, size) in want.items():
    data = inner.read(name)
    if len(data) != size:
        sys.exit('swdata.py: %s is %d bytes, not %d' % (name, len(data), size))
    if name == 'SW.GRP' and zlib.crc32(data) != 0x08A7FA1F:
        sys.exit('swdata.py: SW.GRP is not the shareware 1.2 episode')
    tmp = os.path.join(out, dest + '.part')
    with open(tmp, 'wb') as f:
        f.write(data)
    os.replace(tmp, os.path.join(out, dest))
