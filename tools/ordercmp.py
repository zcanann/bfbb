"""Compare FUNCTION DEFINITION ORDER between our object and the target object.

objdiff pairs symbols by name, so definition order is invisible to it, and a
unit can be 100% on every function while its .text is laid out differently from
the target's. That is one of the things that stops a unit promoting to Matching.

The comparison has to be restricted to the symbols both objects actually
contain. dtk carves the target objects out of the LINKED dol, where CodeWarrior
has already deduplicated weak out-of-line copies of inline functions
(operator=, operator*, ...), so ours legitimately carries extras that the
target's does not. Comparing raw sequences reports order differences that are
nothing but those extras.
"""
import struct
import sys


def funcs(path):
    """[(addr, name)] for STT_FUNC symbols in .text, in address order."""
    d = open(path, "rb").read()
    assert d[:4] == b"\x7fELF", path
    e = ">" if d[5] == 2 else "<"
    shoff = struct.unpack_from(e + "I", d, 0x20)[0]
    shentsize, shnum, shstrndx = struct.unpack_from(e + "HHH", d, 0x2E)
    secs = []
    for i in range(shnum):
        o = shoff + i * shentsize
        nameoff, typ, flags, addr, off, size, link, info, align, ent = \
            struct.unpack_from(e + "10I", d, o)
        secs.append(dict(nameoff=nameoff, type=typ, off=off, size=size,
                         link=link))
    st = secs[shstrndx]
    for s in secs:
        b = st["off"] + s["nameoff"]
        s["name"] = d[b:d.index(b"\0", b)].decode()

    text_idx = {i for i, s in enumerate(secs) if s["name"].startswith(".text")}
    out = []
    for s in secs:
        if s["name"] != ".symtab":
            continue
        strt = secs[s["link"]]
        for i in range(s["size"] // 16):
            o = s["off"] + i * 16
            nameo, value, size, info, other, shndx = \
                struct.unpack_from(e + "IIIBBH", d, o)
            if (info & 0xF) == 2 and shndx in text_idx:
                b = strt["off"] + nameo
                out.append((shndx, value, d[b:d.index(b"\0", b)].decode()))
    out.sort()
    return [(v, n) for _, v, n in out]


for u in sys.argv[1:]:
    short = u.split("/")[-1]
    try:
        t = funcs("build/GQPE78/obj/SB/%s.o" % u)
        o = funcs("build/GQPE78/src/SB/%s.o" % u)
    except Exception as ex:
        print("%-20s ERROR %s" % (short, ex))
        continue

    tn = [n for _, n in t]
    on = [n for _, n in o]
    common = set(tn) & set(on)
    ts = [n for n in tn if n in common]
    os_ = [n for n in on if n in common]

    extra_ours = [n for n in on if n not in common]
    missing = [n for n in tn if n not in common]

    if ts == os_:
        verdict = "order OK"
    else:
        # first genuine divergence
        i = next(k for k in range(min(len(ts), len(os_))) if ts[k] != os_[k])
        verdict = "ORDER DIFFERS at #%d/%d: target %s | ours %s" % (
            i, len(ts), ts[i][:44], os_[i][:44])
    print("%-20s %-4d common  %s" % (short, len(common), verdict))
    if missing:
        print("%-20s   in target only (%d): %s" %
              ("", len(missing), ", ".join(m[:40] for m in missing[:6])))
    if extra_ours:
        print("%-20s   in ours only  (%d): %s" %
              ("", len(extra_ours), ", ".join(m[:40] for m in extra_ours[:6])))
