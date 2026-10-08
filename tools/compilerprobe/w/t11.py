from pad import *
s = base
old = "(_dst) = (RwChar*)RwMalloc(rwstrlen(_src) + 1);"
assert old in s
run(s.replace(old, "(_dst) = (RwChar*)RwMalloc((rwstrlen(_src) + 1) * sizeof(RwChar));"), "sizeof RwChar")
s2 = s.replace("if (_src)  ", "if (((RwChar*)NULL) != (_src))").replace("if (_dst)  ", "if (((RwChar*)NULL) != (_dst))")
print(s2.count("RwChar*)NULL) !="))
run(s2, "NULL != compares")
