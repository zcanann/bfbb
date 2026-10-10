"""Original-only contiguous French particle emitter clusters with typed arrays."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES,TARGET,require
from platforms.france_tu_sequences import compare,digest,unique_template
from platforms.france_hangable_sequence import hangable_pair
from platforms.france_player_animation_context import OriginalData,closed,storage
from platforms.france_update_cull_sequence import checked_identity
from platforms.france_npc_particle_kernels import zero_vector
from platforms.france_dutchman_tweaks import CLUSTER_KIND
from platforms.ps2_source import canonical_linkages
from platforms.ps2_type_layouts import aggregate_layouts

SOURCE='SB/Game/zNPCSupplement.cpp'
GROUPS=[(0x3b48b0,0x3b4c18),(0x3b4de0,0x3b560c)]
MEMBERS=[('NPAR_EmitH2OTrail__FPC5xVec3', 3885232, 204),
 ('NPAR_EmitH2OSpray__FPC5xVec3PC5xVec3', 3885440, 216),
 ('NPAR_EmitH2ODrops__FPC5xVec3PC5xVec3', 3885664, 216),
 ('NPAR_EmitH2ODrips__FPC5xVec3PC5xVec3', 3885888, 216),
 ('NPAR_EmitTubeSparklies__FPC5xVec3PC5xVec3', 3886560, 216),
 ('NPAR_EmitTubeConfetti__FPC5xVec3PC5xVec3', 3886784, 216),
 ('NPAR_EmitTubeSpiralCin__FPC5xVec3PC5xVec3f', 3887008, 396),
 ('NPAR_EmitTubeSpiral__FPC5xVec3PC5xVec3f', 3887408, 396),
 ('NPAR_EmitOilSplash__FPC5xVec3PC5xVec3', 3887808, 216),
 ('NPAR_EmitOilVapors__FPC5xVec3', 3888032, 204),
 ('NPAR_EmitOilTrailz__FPC5xVec3', 3888240, 204),
 ('NPAR_EmitOilShieldPop__FPC5xVec3', 3888448, 204)]
CALLS={3885232: [(132, 1149960), (176, 3901904)],
 3885440: [(140, 1149960), (180, 3901904)],
 3885664: [(140, 1149960), (180, 3901904)],
 3885888: [(140, 1149960), (180, 3901904)],
 3886560: [(140, 1149960), (180, 3911424)],
 3886784: [(140, 1149960), (180, 3911424)],
 3887008: [(156, 1149960), (200, 2024672)],
 3887408: [(156, 1149960), (200, 2024672)],
 3887808: [(140, 1149960), (180, 3917984)],
 3888032: [(132, 1149960), (172, 3917984)],
 3888240: [(132, 1149960), (172, 3917984)],
 3888448: [(132, 1149960), (172, 3917984)]}
OPERANDS={3885232: [(4, 12, 6185760), (152, 168, 5270816), (156, 180, 5212576)],
 3885440: [(24, 28, 6185760), (160, 176, 5270784)],
 3885664: [(24, 28, 6185760), (160, 176, 5270752)],
 3885888: [(24, 28, 6185760), (160, 176, 5270720)],
 3886560: [(24, 28, 6185632), (160, 176, 5270536)],
 3886784: [(24, 28, 6185632), (160, 176, 5270496)],
 3887008: [(32, 40, 6185600), (188, 196, 5270208)],
 3887408: [(32, 40, 6185600), (188, 196, 5270208)],
 3887808: [(24, 28, 6185568), (160, 176, 5270180)],
 3888032: [(4, 12, 6185568), (152, 164, 5270152)],
 3888240: [(4, 12, 6185568), (152, 164, 5270124)],
 3888448: [(4, 12, 6185568), (152, 164, 5270096)]}
INDEPENDENT={2024672: ('SB/Core/x/xMath.cpp', 'xurand', 88),
 3901904: ('SB/Game/zNPCSupplement.cpp', 'ConfigPar', 540),
 3911424: ('SB/Game/zNPCSupplement.cpp', 'ConfigPar', 1144),
 3917984: ('SB/Game/zNPCSupplement.cpp', 'ConfigPar', 304)}

ARRAYS=[('g_npar_mgmt','NPARMgmt',12,32,0x5e6240),
        ('g_parm_chucksplash','NPARParmChuckSplash',5,32,0x506ca0),
        ('g_parm_tubeconfetti','NPARParmTubeConfetti',2,40,0x506be0),
        ('g_parm_tubespiral','NPARParmTubeSpiral',4,12,0x506ac0),
        ('g_parm_oilbub','NPARParmOilBub',4,28,0x506a50)]


def original_arrays(original,target):
    data=OriginalData(original)
    layouts=aggregate_layouts(data.debug,SOURCE,{a[1] for a in ARRAYS}|{'NPARData'})
    mgmt=layouts['NPARMgmt'];particle=layouts['NPARData']
    require(mgmt['size']==32 and [(m['name'],m['offset']) for m in mgmt['members']]==
            [('typ_npar',0),('flg_npar',4),('par_buf',8),('cnt_active',12),('num_max',16),
             ('txtr',20),('xtra_data',24),('user_data',28)],'Emitter management field layout differs')
    members={m['name']:m for m in mgmt['members']}
    require(all(members[n]['type_attributes']=={'5':8} for n in ('flg_npar','cnt_active','num_max')) and
            members['par_buf']['type_attributes']=={'8':'01'+particle['die_offset'].to_bytes(4,'little').hex()} and
            particle['size']==80,'Emitter management count/buffer types differ')
    records=[]
    for name,typ,count,stride,b in ARRAYS:
        off,decl,a=data.declaration(SOURCE,name);tag,_,array=data.by[decl.get(7)]
        desc=array.get(10);layout=layouts[typ]
        expected=bytes.fromhex('000a0000000000')+bytes([count-1])+bytes.fromhex('000000087200')+layout['die_offset'].to_bytes(4,'little')
        require(tag==1 and array.get(9)==0 and desc==expected and layout['size']==stride,
                'Emitter original array count, type or element extent differs')
        size=count*stride;kind='zero_fill' if name=='g_npar_mgmt' else 'initialized_data'
        for binary,address in ((original,a),(target,b)):storage(binary,address,size,'runtime_bss' if kind=='zero_fill' else kind)
        payload=None
        if kind=='initialized_data':
            require(original.read(a,size)==target.read(b,size),'Complete emitter parameter array payload differs')
            payload=digest(target.read(b,size))
        records.append({'name':name,'reference_address':a,'target_address':b,'size':size,'count':count,
                        'stride':stride,'type':typ,'declaration_die':off,'type_die':decl[7],
                        'subscript_descriptor':desc.hex(),'layout':layout,'storage':kind,
                        'payload_sha256':payload,'data_extent_promoted':False})
    require(all(a['target_address']+a['size']<=b['target_address'] or b['target_address']+b['size']<=a['target_address']
                for i,a in enumerate(records) for b in records[i+1:]),'Emitter array ownership overlaps')
    return records,zero_vector(original,target)


def generate_unit(originals,registry_dir):
    target=originals[TARGET];known={}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text())['functions']:
            if f['address'] in INDEPENDENT:
                old=known.setdefault(f['address'],f)
                require(all(old[k]==f[k] for k in ('source','name','size','sha256')),'Conflicting emitter dependency')
    require(set(known)==set(INDEPENDENT),'Independent emitter dependency missing')
    for address,identity in INDEPENDENT.items():
        require(tuple(known[address][k] for k in ('source','name','size'))==identity,'Emitter dependency identity differs')
    records={};sequences=[];data_proofs=[];call_proofs=[]
    for version in REFERENCES:
        original=originals[version];links=canonical_linkages(original.data,original.metadata)
        arrays,zero=original_arrays(original,target);matched={};operands=[];transfers=[];contexts=[]
        for linkage,b,size in MEMBERS:
            refs=[f for f in original.functions if f['source']==SOURCE and links.get(f['low'])==linkage]
            require(len(refs)==1 and refs[0]['name']==linkage.split('__')[0] and
                    refs[0]['high']-refs[0]['low']==size,'Complete emitter original identity differs')
            f=refs[0];a=f['low'];require(a%16==b%16==0,'Emitter entry alignment differs')
            padding=(-size)%16
            require(not any(original.read(a+size,padding)) and not any(target.read(b+size,padding)), 'Emitter zero padding differs')
            pairs,calls,masks=compare(original,target,a,b,size,address_resolver=hangable_pair)
            require([(p['hi_offset'],p['lo_offset'],p['target_address']) for p in pairs]==OPERANDS[b],
                    'Emitter data inventory differs')
            expected={}
            for p in pairs:
                require(p['opcode']==9,'Emitter data operand is not an address')
                if p['target_address']==zero['target_address']:
                    require(p['reference_address']==zero['reference_address'],'Emitter zero vector owner differs')
                    path=['g_O3'];kind='file_backed'
                else:
                    candidates=[r for r in arrays if r['target_address']<=p['target_address']<r['target_address']+r['size']]
                    require(len(candidates)==1,'Emitter operand lacks a complete typed array')
                    owner=candidates[0];offset=p['target_address']-owner['target_address']
                    require(offset%owner['stride']==0 and p['reference_address']==owner['reference_address']+offset,
                            'Emitter operand does not select the same original array element')
                    path=[owner['name'],offset//owner['stride']];kind='zero_fill' if owner['name']=='g_npar_mgmt' else 'file_backed'
                require(p['storage']==kind,'Emitter operand storage differs')
                expected[p['lo_offset']]=0xffff0000
                if original.read(a+p['hi_offset'],4)!=target.read(b+p['hi_offset'],4):expected[p['hi_offset']]=0xffff0000
                operands.append({'caller':linkage,'caller_address':b,'path':path,**p})
            require([(c['offset'],c['target_address']) for c in calls]==CALLS[b] and all(c['opcode']==3 for c in calls),
                    'Emitter complete call inventory differs')
            for c in calls:
                ra,tb=c['reference_address'],c['target_address']
                if tb==0x118c08:
                    require(ra==tb and ra not in original.by_address and original.read(ra,64)==target.read(tb,64) and
                            original.word(a+c['offset'])==target.word(b+c['offset']), 'Emitter literal opaque runtime context differs')
                    masks.pop(c['offset'])
                    contexts.append({'caller':linkage,'caller_address':b,**c,'transfer_word_unmasked':True,
                                     'opaque_context_bytes':64,'opaque_context_sha256':digest(target.read(tb,64)),
                                     'no_identity_or_extent_claim':True})
                else:
                    evidence=checked_identity(original,target,ra,tb,known);expected[c['offset']]=0xfc000000
                    transfers.append({'caller':linkage,'caller_address':b,**c,
                                      'callee':{k:evidence[k] for k in ('source','name','address','size','sha256')}})
            require(masks==expected,'Emitter masks exceed typed arrays and complete calls')
            boundary=closed(original,target,a,b,size);matched[b]=(a,size,masks)
            if b not in records:
                records[b]={'name':f['name'],'source':SOURCE,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                            'boundary_confirmation':True,'confirmation_kind':CLUSTER_KIND,'provenance':[],
                            'corroboration':{'proof_scope':'complete_original_ordered_particle_emitter_clusters',
                                            'local_control_flow':boundary,'whole_translation_unit_claimed':False}}
            records[b]['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                            'source':SOURCE,'name':f['name'],'linkage_name':linkage,
                                            'reference_sha256':digest(original.read(a,size)),
                                            'data_address_operands':pairs,'direct_transfers':calls})
        groups=[]
        for b,end in GROUPS:
            members=[(tb,*matched[tb]) for tb in matched if b<=tb<end];a=members[0][1];size=end-b
            original_members=sorted((f['low'],f['high']-f['low']) for f in original.functions if a<=f['low']<a+size)
            require(original_members==[(ra,n) for tb,ra,n,m in members] and
                    all(ra-a==tb-b for tb,ra,n,m in members) and members[-1][0]+members[-1][2]==end,
                    'Complete original emitter group membership/order differs')
            masks={tb-b+off:mask for tb,ra,n,m in members for off,mask in m.items()}
            unique=unique_template(target,original.read(a,size),masks,b)
            groups.append({'source_address':a,'target_address':b,'size':size,'member_addresses':[m[0] for m in members],
                           'uniqueness':unique,'terminal_alignment_bytes':(-size)%16})
        sequences.append({'version':version,'complete_groups':groups,'whole_translation_unit_claimed':False})
        data_proofs.append({'version':version,'complete_typed_arrays':arrays,'zero_vector':zero,'operands':operands})
        call_proofs.append({'version':version,'complete_callees':transfers,'opaque_runtime_contexts':contexts})
    return {'functions':sorted(records.values(),key=lambda f:f['address']),'sequence_proofs':sequences,
            'data_proofs':data_proofs,'call_proofs':call_proofs,
            'counts':{'functions':12,'code_bytes':2904,'source_units':1,'closed_return_bodies':12,
                      'reviewed_complete_caller_callee_clusters':2}}
