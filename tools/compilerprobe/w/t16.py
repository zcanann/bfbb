from cx import *
s = base
old = "(_dst) = (RwChar*)RwMalloc(rwstrlen(_src) + 1);"
a = s.replace(old, "(_dst) = (RwChar*)RwMalloc((rwstrlen(_src) + 1) * sizeof(RwChar));")
measure(a, "sizeof RwChar")
b = s.replace("if (_src)  ", "if (((RwChar*)NULL) != (_src))").replace("if (_dst)  ", "if (((RwChar*)NULL) != (_dst))")
measure(b, "NULL != in macro")
c = a.replace("if (_src)  ", "if (((RwChar*)NULL) != (_src))").replace("if (_dst)  ", "if (((RwChar*)NULL) != (_dst))")
measure(c, "both")
