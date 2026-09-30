#!/usr/bin/env python3
"""Address-normalised side-by-side diff of one function, target vs ours.

objdump prints absolute offsets that differ between the two objects for every
function, so a raw diff reports every line as changed. This strips the address
column, rewrites intra-function branch targets to be relative, and erases the
per-TU anonymous pool ordinals (@NNN / $NNN) that carry no meaning.

Usage:  fndiff.py <unit-path-fragment> <mangled-function-name>
        fndiff.py SB/Game/zTalkBox trigger_sound__22@unnamed@zTalkBox_cpp@FRC...
"""
import os, re, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OD = os.path.join(ROOT, "build", "binutils", "powerpc-eabi-objdump.exe")


def body(obj, fn):
    out = subprocess.run([OD, "-d", "-r", "-M", "broadway", obj],
                         capture_output=True, text=True).stdout
    lines, inside, base = [], False, None
    for l in out.splitlines():
        m = re.match(r"^([0-9a-f]+) <(.+)>:$", l)
        if m:
            if inside:
                break
            if m.group(2) == fn:
                inside, base = True, int(m.group(1), 16)
            continue
        if not inside:
            continue
        m = re.match(r"^\s*([0-9a-f]+):\t(?:[0-9a-f]{2} ){4}\t(.*)$", l)
        if m:
            txt = m.group(2)
            # intra-function branch -> +N relative to this instruction
            here = int(m.group(1), 16)
            def rel(mm):
                return "<%+d>" % (int(mm.group(1), 16) - here)
            txt = re.sub(r"\b([0-9a-f]+) <" + re.escape(fn) + r"(?:\+0x[0-9a-f]+)?>", rel, txt)
            lines.append(re.sub(r"[@$]\d+", "@P", txt))
        else:
            m = re.search(r"(R_PPC_\S+)\s+(\S+)", l)
            if m:
                lines.append("\t%s %s" % (m.group(1), re.sub(r"[@$]\d+", "@P", m.group(2))))
    return lines


def main():
    frag, fn = sys.argv[1], sys.argv[2]
    t = body(os.path.join(ROOT, "build/GQPE78/obj", frag + ".o"), fn)
    o = body(os.path.join(ROOT, "build/GQPE78/src", frag + ".o"), fn)
    if not t:
        sys.exit("function not found in target object")
    import difflib
    for l in difflib.unified_diff(t, o, "target", "ours", lineterm="", n=4):
        print(l)


main()
