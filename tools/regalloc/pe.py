import struct
class PE:
    def __init__(s, path):
        b = s.b = open(path,'rb').read()
        e = struct.unpack_from('<I', b, 0x3c)[0]
        nsec = struct.unpack_from('<H', b, e+6)[0]
        osz = struct.unpack_from('<H', b, e+20)[0]
        s.base = struct.unpack_from('<I', b, e+24+28)[0]
        s.secs=[]
        o = e+24+osz
        for i in range(nsec):
            name=b[o:o+8].rstrip(b'\0').decode()
            vsz,va,rsz,raw=struct.unpack_from('<IIII', b, o+8)
            s.secs.append((name,va,vsz,raw,rsz)); o+=40
    def off2va(s, off):
        for n,va,vsz,raw,rsz in s.secs:
            if raw<=off<raw+rsz: return s.base+va+off-raw
    def va2off(s, va):
        r=va-s.base
        for n,sva,vsz,raw,rsz in s.secs:
            if sva<=r<sva+max(vsz,rsz): return raw+r-sva
    def read(s, va, n): o=s.va2off(va); return s.b[o:o+n]
    def text(s):
        for n,va,vsz,raw,rsz in s.secs:
            if n=='.text': return s.base+va, s.b[raw:raw+rsz]
