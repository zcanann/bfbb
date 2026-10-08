from cx import *
from pad import run
s = base
mac = s.replace("if (_src)  ", "if (((RwChar*)NULL) != (_src))").replace("if (_dst)  ", "if (((RwChar*)NULL) != (_dst))")
def rep(t, pairs):
    for a,b in pairs:
        assert a in t, a
        t = t.replace(a,b,1)
    return t
destr = [("    if (userData->name)\n    {\n        RwFree", "    if (NULL != userData->name)\n    {\n        RwFree"),
         ("            if (charData[i])\n            {\n                RwFree", "            if (NULL != charData[i])\n            {\n                RwFree"),
         ("    if (userData->data)\n    {\n        RwFree", "    if (NULL != userData->data)\n    {\n        RwFree"),
         ("    if (list->userData)\n    {\n        for", "    if (NULL != list->userData)\n    {\n        for")]
udc = [("    if (srcUserData->name)", "    if (NULL != srcUserData->name)"),
       ("    if (srcUserData->data)", "    if (NULL != srcUserData->data)"),
       ("                if (!srcCharData[i])", "                if (NULL == srcCharData[i])")]
for tag, t in [("destr only", rep(s, destr)), ("udc only", rep(s, udc)), ("mac+destr", rep(mac, destr)), ("mac+udc", rep(mac, udc)), ("all", rep(mac, destr+udc))]:
    measure(t, tag)
    run(t, tag)
