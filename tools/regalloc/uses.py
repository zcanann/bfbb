import json,sys
c=[x for x in json.load(open(sys.argv[1])) if x['cls']==int(sys.argv[2])][0]
ops={int(k):v for k,v in json.load(open('opmap.json')).items()}
want=set(int(x) for x in sys.argv[3].split(','))
for bi,b in enumerate(c['blocks']):
    for ii,(op,fl,args) in enumerate(b):
        rv=[a[2] for a in args if a[0]==0 and a[1]==c['cls']]
        if want & set(rv):
            print(bi,ii,ops.get(op), [('v%d'%a[2] if a[0]==0 else a[3]) for a in args])
