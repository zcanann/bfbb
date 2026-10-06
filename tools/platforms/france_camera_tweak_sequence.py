"""Original-only French camera-tweak TU, typed state, and two exact base tails."""
from __future__ import annotations
import json
from platforms.verify_reviewed import REFERENCES, TARGET, require
from platforms.france_corroborated import ControlFlow
from platforms.france_tu_sequences import KIND, compare, digest, unique_template, words, gpr_writes, named_data
from platforms.france_update_cull_sequence import checked_identity
from platforms.ps2_source import canonical_linkages
from platforms.dwarf1 import iter_dies
from platforms.ps2_type_layouts import aggregate_layouts
SOURCE = 'SB/Game/zCameraTweak.cpp'
START = 3881232
OPAQUE = (1132176,1132848,1133080)

def camera_gpr_writes(word):
    # The observed SUB.S, SQRT.S, MULA.S and MADD.S write only FPR/ACC state.
    # COP1 transfers have a different rs encoding and retain the strict decoder.
    if word >> 26 == 17 and word >> 21 & 31 == 16 and word & 63 in (1,4,26,28):
        return set()
    return gpr_writes(word)

MEMBERS = [('zCameraTweak_EventCB', 184), ('zCameraTweak_Load', 8), ('zCameraTweak_Save', 8), ('zCameraTweak_Init', 92), ('zCameraTweakGlobal_GetPitch', 8), ('zCameraTweakGlobal_GetH', 8), ('zCameraTweakGlobal_GetD', 8), ('zCameraTweakGlobal_Update', 244), ('zCameraTweakGlobal_Reset', 52), ('zCameraTweakGlobal_Remove', 296), ('zCameraTweakGlobal_Add', 536), ('zCameraTweakGlobal_Init', 240)]

def camera_pair(body, address, offset, *, allow_return_store=False):
    """Find a non-clobbered LUI reaching this actual ADDIU/LW/SW/LWC1/SWC1 operand.

    Calls may occur only immediately before LO: its delay slot executes before
    the callee. Direct edges cannot bypass LUI, except a branch whose own delay
    slot is that LUI (both taken edges execute it). No indirect transfers pass.
    A separate opt-in permits a final JR $ra followed immediately by a low SW
    address consumer, with a non-$ra address register. Strict callers reject it.
    """
    code = words(body)
    low = code[offset // 4]
    register = low >> 21 & 31
    require(low >> 26 in (9, 35, 43, 49, 57) and register != 0, f'Not an eligible address operand: {address:#x}+{offset:#x}: {low:#x}')
    high_offset = None
    for off in range(offset - 4, -1, -4):
        word = code[off // 4]
        if register in camera_gpr_writes(word):
            require(word >> 26 == 15 and word >> 21 & 31 == 0,
                    'Address register is not produced by LUI')
            high_offset = off
            break
    require(high_offset is not None, 'Address operand lacks LUI definition')
    flow = ControlFlow({address + i * 4: w for i, w in enumerate(code)})
    if high_offset >= 4:
        previous = flow.instruction(address + high_offset - 4)
        require(previous['kind'] == 'normal' or
                (previous['kind'] == 'branch' and not previous.get('likely')),
                'LUI in an annulled or call delay slot needs a separate lifetime proof')
    for off in range(high_offset + 4, offset, 4):
        instruction = flow.instruction(address + off)
        require(instruction['kind'] in ('normal', 'branch') or
                (instruction['kind'] == 'call' and off == offset - 4 and register != 31) or
                (allow_return_store and instruction['word'] == 0x03e00008 and
                 off == offset - 4 and offset == len(body) - 4 and low >> 26 == 43 and register != 31),
                'Call or unsupported transfer interrupts address lifetime')
    for off in range(0, len(body), 4):
        instruction = flow.instruction(address + off)
        destination = instruction.get('target')
        if destination is not None and address + high_offset < destination <= address + offset:
            require(off == high_offset - 4 and instruction['kind'] == 'branch',
                    'A direct edge bypasses the address definition')
    high = code[high_offset // 4]
    immediate = (low & 65535) - (65536 if low & 32768 else 0)
    return high_offset, (((high & 65535) << 16) + immediate) & 0xffffffff

def original_data(original):
    section = next(s for s in original.metadata['sections'] if s['name']=='.debug' and s['size'])
    debug = original.data[section['offset']:section['offset']+section['size']]
    dies = list(iter_dies(debug)); by = {off:(tag,owner,a) for off,tag,owner,a in dies}
    layouts = aggregate_layouts(debug,SOURCE,{'zCamTweak','zCamTweakLook'})
    expected = {'zCamTweak':(20,{'owner':(0,9),'priority':(4,14),'time':(8,14),'pitch':(12,14),'distMult':(16,14)}),
                'zCamTweakLook':(12,{'h':(0,14),'dist':(4,14),'pitch':(8,14)})}
    for name,(size,fields) in expected.items():
        layout = layouts[name]
        require(layout['size']==size and {m['name']:(m['offset'],m['type_attributes']) for m in layout['members']}==
                {name:(offset,{'5':fundamental}) for name,(offset,fundamental) in fields.items()},
                'Original camera state member offsets or fundamental types differ')
    globals = []
    for name,size in [('sCamTweakList',160),('zcam_neartweak',12),('zcam_fartweak',12)]:
        decl = [(off,a) for off,tag,owner,a in dies if tag in (7,12) and owner.replace('\\','/').endswith(SOURCE) and a.get(3)==name]
        require(len(decl)==1,'Ambiguous original camera state declaration')
        off,attrs = decl[0]; location = attrs.get(2); typ = attrs.get(7)
        require(isinstance(location,bytes) and len(location)==5 and location[0]==3,'Camera state lacks absolute original address')
        type_proof = {'type_die':typ}
        if name=='sCamTweakList':
            tag,owner,array = by[typ]; desc = array.get(10)
            require(tag==1 and array.get(9)==0 and isinstance(desc,bytes) and len(desc)==18 and
                    desc[:14]==bytes.fromhex('000a000000000007000000087200') and
                    int.from_bytes(desc[14:],'little')==layouts['zCamTweak']['die_offset'],
                    'Camera list is not the original eight-element typed array')
            type_proof.update(array_count=8,element_size=20,descriptor=desc.hex())
        else:
            require(typ==layouts['zCamTweakLook']['die_offset'],'Camera look global has a different original type')
        globals.append({'name':name,'reference_address':int.from_bytes(location[1:],'little'),
                        'size':size,'declaration_die':off,**type_proof})
    return globals,layouts


def tail_bounds(original,target,reference,address,name,calls,known):
    require(name in ('zCameraTweak_Load','zCameraTweak_Save') and len(calls)==1,
            'Unreviewed camera tail')
    call = calls[0]
    require(call['offset']==0 and call['opcode']==2,'Camera tail is not a sole terminal J')
    callee = checked_identity(original,target,call['reference_address'],call['target_address'],known)
    expected = 'xBaseLoad' if name.endswith('Load') else 'xBaseSave'
    require(callee['name']==expected and callee['source']=='SB/Core/x/xBase.cpp',
            'Camera tail changes original base ownership')
    for binary,entry,destination in ((original,reference,call['reference_address']),
                                     (target,address,call['target_address'])):
        require(binary.word(entry)>>26==2 and binary.word(entry+4)==0 and
                (((entry+4)&0xf0000000)|((binary.word(entry)&0x3ffffff)<<2))==destination,
                'Camera tail differs from its original J/NOP')
        cf = ControlFlow({destination+4*i:w for i,w in enumerate(words(binary.read(destination,callee['size']+16)))})
        require(cf.bounds(destination,callee['size'])['passes'],'Base tail destination must strictly close')
    return {'passes':True,'boundary_kind':'reviewed-exact-eight-byte-j-nop-tail',
            'tail_target_name':expected,'tail_target_address':call['target_address'],
            'callee_size':callee['size'],'both_original_callees_closed':True,'unchanged_sp_ra':True}


def generate_unit(originals,registry_dir):
    target = originals[TARGET]; known = {}
    for path in sorted(registry_dir.glob('*functions.json')):
        for f in json.loads(path.read_text()).get('functions',[]):
            if f['source']==SOURCE and f['name']!='zCameraTweakGlobal_Reset':
                continue
            if f['address'] in known:
                require(all(known[f['address']][k]==f[k] for k in ('name','source','size','sha256')),
                        'Conflicting independent camera context')
            known[f['address']] = f
    anchors = [f for f in known.values() if f['source']==SOURCE]
    require(len(anchors)==1 and anchors[0]['name']=='zCameraTweakGlobal_Reset' and anchors[0]['size']==52,
            'Expected the previously confirmed complete Reset neighbor')
    functions,sequences,data_proofs,contexts = {},[],[],[]
    for version in REFERENCES:
        original = originals[version]
        members = sorted((f for f in original.functions if f['source']==SOURCE),key=lambda f:f['low'])
        require([(f['name'],f['high']-f['low']) for f in members]==MEMBERS,'Original complete camera membership changed')
        low,end = members[0]['low'],members[-1]['high'];shift = START-low
        require(end-low==1776,'Camera original sequence/alignment changed')
        reset = next(f for f in members if f['name']=='zCameraTweakGlobal_Reset')
        checked_identity(original,target,reset['low'],reset['low']+shift,known)
        globals,layouts = original_data(original)
        inside = {f['low']:f for f in members};linkages = canonical_linkages(original.data,original.metadata)
        masks,rows,mapping,pairmap,callees = {},[],{},{},{}
        for i,f in enumerate(members):
            a,b,size,name = f['low'],f['low']+shift,f['high']-f['low'],f['name']
            require(a in linkages and a%16==b%16==0,'Original camera linkage/alignment changed')
            if i:
                gap = a-members[i-1]['high']
                require(0<=gap<16 and not any(original.read(a-gap,gap)) and not any(target.read(b-gap,gap)),
                        'Camera alignment bytes changed')
            pairs,calls,mask = compare(original,target,a,b,size,address_resolver=camera_pair)
            bounds = None
            for binary,entry in ((original,a),(target,b)):
                cf = ControlFlow({entry+4*j:w for j,w in enumerate(words(binary.read(entry,size+16)))})
                bounds = cf.bounds(entry,size)
                if not bounds['passes']:
                    require(size==8,'Only reviewed eight-byte camera tails may leave their extents')
                    bounds = tail_bounds(original,target,a,b,name,calls,known)
            for pair in pairs:
                ra,tb = pair['reference_address'],pair['target_address']
                require(ra not in pairmap or pairmap[ra]==tb,'Camera data mapping is inconsistent')
                pairmap[ra] = tb
                if ra in inside:
                    require(tb==ra+shift and inside[ra]['name']=='zCameraTweak_EventCB' and pair['opcode']==9 and
                            named_data(original,'zCameraTweak_EventCB',SOURCE)==ra,
                            'Callback pointer must reference the complete original EventCB')
                    continue
                owners = [g for g in globals if g['reference_address']<=ra<g['reference_address']+g['size']]
                require(len(owners)==1,'Unproved camera state operand')
                g = owners[0];off = ra-g['reference_address'];base = tb-off
                require(g['name'] not in mapping or mapping[g['name']]==base,'Camera aggregate component bases disagree')
                mapping[g['name']] = base
                if pair['opcode'] in (49,57):
                    require((off%20 in (4,8,12,16)) if g['name']=='sCamTweakList' else off in (0,4,8),
                            'Float state access does not reference an original float member')
                for binary,start in ((original,g['reference_address']),(target,base)):
                    region = binary._stream_regions['runtime_bss']
                    require(region['address']<=start and start+g['size']<=region['address']+region['size'],
                            'Complete original typed camera state escapes BSS')
            for call in calls:
                ra,tb = call['reference_address'],call['target_address']
                require(ra not in callees or callees[ra]==tb,'Camera callee relationships disagree')
                callees[ra] = tb
                if ra in inside:
                    require(tb==ra+shift,'Camera internal call changed full member ownership')
                elif tb in known:
                    checked_identity(original,target,ra,tb,known)
                else:
                    require(ra==tb and tb in OPAQUE and call['opcode']==3 and
                            original.word(a+call['offset'])==target.word(b+call['offset']) and
                            original.read(ra,64)==target.read(tb,64),'Opaque runtime caller or complete context changed')
                    mask.pop(call['offset'],None)
                    call.update(opaque_context_bytes=64,opaque_context_sha256=digest(target.read(tb,64)),
                                caller_word_unmasked=True,no_identity_or_extent_claim=True)
                    contexts.append({'version':version,'function_address':b,**call,'promoted_as_named_anchor':False})
            masks.update({a-low+k:w for k,w in mask.items()})
            rows.append({'name':name,'address':b,'size':size,'pairs':pairs,'calls':calls})
            if b not in known:
                if b not in functions:
                    functions[b] = {'name':name,'source':SOURCE,'address':b,'size':size,'sha256':digest(target.read(b,size)),
                                    'boundary_confirmation':True,'confirmation_kind':KIND,'provenance':[],
                                    'corroboration':{'local_control_flow':bounds,'previously_confirmed_neighbor_entries':[anchors[0]['address']]}}
                rec = functions[b]
                require((rec['name'],rec['size'])==(name,size) and (not rec['provenance'] or rec['provenance'][0]['linkage_name']==linkages[a]),
                        'Original camera identities disagree')
                rec['provenance'].append({'version':version,'executable_sha1':original.sha1,'source_address':a,
                                         'name':name,'source':SOURCE,'linkage_name':linkages[a],
                                         'reference_sha256':digest(original.read(a,size)),
                                         'data_address_operands':pairs,'direct_transfers':calls})
        require(len(mapping)==3 and len(set(mapping.values()))==3 and len(set(pairmap.values()))==len(pairmap) and
                len(set(callees.values()))==len(callees),'Distinct camera state/callees collapse')
        sequences.append({'version':version,'source':SOURCE,'source_start':low,'target_start':START,
                          'complete_original_members':12,'previously_confirmed_members':1,'sequence_bytes_with_alignment':1776,
                          'uniqueness':unique_template(target,original.read(low,end-low),masks,START)})
        data_proofs.append({'version':version,'source':SOURCE,'original_aggregate_layouts':layouts,
                            'typed_globals':[{**g,'target_address':mapping[g['name']]} for g in globals],
                            'member_operands_and_calls':rows,'data_extents_promoted':False})
    require(len(functions)==11 and sum(f['size'] for f in functions.values())==1632 and
            all(len(f['provenance'])==3 for f in functions.values()),'All eleven camera members require all three originals')
    return {'functions':sorted(functions.values(),key=lambda f:f['address']),'sequence_proofs':sequences,
            'data_proofs':data_proofs,'call_neighbors':contexts,
            'counts':{'functions':11,'code_bytes':1632,'source_units':1,'closed_return_bodies':9,'reviewed_camera_tweak_tails':2}}
