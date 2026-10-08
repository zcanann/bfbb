from pad import *
old = '''    UserDataListCopy(RPUSERDATALISTGETDATA(dstObject, offset),
                     RPUSERDATALISTGETDATA(srcObject, offset));
'''
for k in [1,3,6,10,20]:
    run(base.replace(old, "    ((RpUserDataList*)dstObject)->numElements = 0;\n"*k + old), "objcopy pre-pad %d"%k)
