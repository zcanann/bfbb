"""verify.py [units...]: the instrumented (log-mode) compile must be byte-identical to a plain one."""
import sys, os, json, hashlib, tempfile
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import aaudit, qprobe, rulesnap
from concurrent.futures import ThreadPoolExecutor
us = rulesnap.units("all")
if len(sys.argv) > 1:
    us = [u for u in us if any(x in u["name"] for x in sys.argv[1:])]
def one(u):
    r = []
    for js in ("send({nabl:0});recv('ack',function(){}).wait();", aaudit.js(None, True)):
        out = os.path.join(tempfile.mkdtemp(prefix="vf_"), "o.o")
        aaudit.compile_unit(u, out, js, aaudit.DEFMW)
        r.append(hashlib.sha1(open(out, "rb").read()).hexdigest() if os.path.exists(out) else None)
    return u["name"], r
with ThreadPoolExecutor(16) as ex:
    bad = [(n, r) for n, r in ex.map(one, us) if r[0] != r[1] or r[0] is None]
print(len(us), "units;", len(bad), "differ:", bad[:20])
