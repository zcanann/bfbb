from pad import *
old = '''    UserDataListCopy(RPUSERDATALISTGETDATA(dstObject, offset),
                     RPUSERDATALISTGETDATA(srcObject, offset));
'''
new = '''    RpUserDataList* dstUserDataList;
    const RpUserDataList* srcUserDataList;

    dstUserDataList = RPUSERDATALISTGETDATA(dstObject, offset);
    srcUserDataList = RPUSERDATALISTGETDATA(srcObject, offset);

    UserDataListCopy(dstUserDataList, srcUserDataList);
'''
assert old in base
run(base.replace(old,new), "objcopy locals")
