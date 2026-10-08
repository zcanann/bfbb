from pad import *
s = base
old_destroy_tail = '''        RwFree(list->userData);
    }

    list->userData = (RpUserDataArray*)NULL;
    list->numElements = 0;
}
'''
assert old_destroy_tail in s
cons = '''static void UserDataListConstruct(RpUserDataList* userDataList)
{
    userDataList->numElements = 0;
    userDataList->userData = (RpUserDataArray*)NULL;
}

'''
s1 = s.replace('static void UserDataListDestroy(', cons + 'static void UserDataListDestroy(')
s1 = s1.replace(old_destroy_tail, '''        RwFree(list->userData);
    }

    UserDataListConstruct(list);
}
''')
run(s1, "destroy->construct")
s2 = s.replace('static void UserDataListDestroy(', cons + 'static void UserDataListDestroy(')
s2 = s2.replace('''    UserDataListDestroy(dstList);
''', '''    UserDataListDestroy(dstList);
    UserDataListConstruct(dstList);
''')
run(s2, "listcopy destroy+construct")
