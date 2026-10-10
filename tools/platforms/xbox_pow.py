"""Prove the original x87 pow wrapper through the pinned vendor default implementation.

Only the closed 27-byte wrapper enters coverage. Its 529-byte context preserves
the original two-byte entry jump and complete vendor implementation; genuine
COFF relocation fields are the only masked bytes. Context callees stay unnamed.
"""
import hashlib
from pathlib import Path
import struct
from .xbox_runtime import require

VENDOR = {
    'archive_sha256': '780aa4cbe614efeeb72e3bb9538230f5cd37caeb0adc5d70c346b0dda19ba3a5',
    'member': '..\\build\\intel\\mt_obj\\pow.obj',
    'member_sha256': 'c887f7e9951109ccda5f2ea7e9492fda5f34faa1ff308cd85111f52228b65ce6',
    'symbol': '__CIpow', 'symbol_offset': 64, 'symbol_size': 59,
    'default_symbol': '__CIpow_default', 'default_offset': 123, 'default_size': 527,
    'wrapper_size': 27, 'original_entry_jump': 'eb00',
    'normalized_sha256': '6ec0368b71b43cb56dbaff6ea702d9d438691f5f350eff4ebcbfd74547070e22',
    'relocations': [[30,'__fload_withFB',20],[50,'__load_CW',20],
        [77,'__fload_withFB',20],[114,'__twoToTOS',20],[127,'___fastflag',6],
        [134,'__fast_exit',20],[140,'POW_name',6],[150,'__check_range_exit',20],
        [156,'___fastflag',6],[163,'__fast_exit',20],[169,'POW_name',6],
        [179,'__startTwoArgErrorHandling',20],[190,'__fload_withFB',20],
        [251,'__fload_withFB',20],[310,'__powhlp',20],[330,'__fast_exit',20],
        [404,'__infinity',6],[430,'__fast_exit',20],[437,'__fast_exit',20],
        [446,'__fast_exit',20],[473,'__indefinite',6],[503,'__half',6]],
}


def verify_pow_vendor(library: Path) -> dict:
    archive = library.read_bytes()
    require(hashlib.sha256(archive).hexdigest()==VENDOR['archive_sha256'] and
            archive[:8]==b'!<arch>\n', 'Unexpected pow vendor archive')
    cursor, strings, selected = 8, b'', []
    while cursor<len(archive):
        header=archive[cursor:cursor+60]
        require(len(header)==60 and header[58:]==b'`\n','Invalid pow archive member')
        size=int(header[48:58]); name=header[:16].decode('ascii').strip()
        require(cursor+60+size<=len(archive),'Truncated pow archive member')
        body=archive[cursor+60:cursor+60+size]
        if name=='//': strings=body
        elif name.startswith('/') and name[1:].isdigit():
            offset=int(name[1:]); require(offset<len(strings),'Invalid pow member name')
            name=strings[offset:strings.index(b'\0',offset)].decode('ascii')
        if name==VENDOR['member']: selected.append(body)
        cursor+=60+size+size%2
    require(len(selected)==1 and hashlib.sha256(selected[0]).hexdigest()==VENDOR['member_sha256'],
            'Missing or changed vendor pow object')
    body=selected[0]
    machine,count,_,symbols_at,symbol_count,optional,_=struct.unpack_from('<HHIIIHH',body)
    require(machine==0x14c and optional==0 and count>=1,'Unexpected pow COFF')
    strings_at=symbols_at+18*symbol_count; symbols={}; index=0
    while index<symbol_count:
        raw,value,section,kind,storage,aux=struct.unpack_from('<8sIhHBB',body,symbols_at+18*index)
        if raw[:4]==bytes(4):
            offset=strings_at+struct.unpack_from('<I',raw,4)[0]
            name=body[offset:body.index(b'\0',offset)].decode('ascii')
        else: name=raw.rstrip(b'\0').decode('ascii')
        symbols[index]={'name':name,'value':value,'section':section,'kind':kind,'storage':storage,
                        'aux':aux,'size':struct.unpack_from('<I',body,symbols_at+18*(index+1)+4)[0] if aux else None}
        index+=1+aux
    for name,offset,size in ((VENDOR['symbol'],64,59),(VENDOR['default_symbol'],123,527)):
        found=[s for s in symbols.values() if s['name']==name]
        require(found==[{'name':name,'value':offset,'section':1,'kind':0x20,'storage':2,'aux':1,'size':size}],
                'Vendor pow named entry or auxiliary extent differs')
    section=struct.unpack_from('<8sIIIIIIHHI',body,20)
    require(section[0].rstrip(b'\0')==b'.text' and section[3]==650,'Vendor pow code section differs')
    text=body[section[4]:section[4]+650]
    # The public intrinsic's disabled-SSE2 branch reaches this named default.
    require(text[64:73]==bytes.fromhex('833d00000000007432') and 64+9+0x32==123,
            'Public pow intrinsic does not reach the reviewed x87 default')
    code=bytearray(text[123:650]); relocations=[]
    for i in range(section[7]):
        offset,target,kind=struct.unpack_from('<IIH',body,section[5]+10*i)
        if 123<=offset<650:
            require(offset+4<=650 and code[offset-123:offset-119]==bytes(4),'Unexpected pow vendor addend')
            relocations.append([offset-123,symbols[target]['name'],kind])
    require(relocations==VENDOR['relocations'] and hashlib.sha256(code).hexdigest()==VENDOR['normalized_sha256'],
            'Vendor pow bytes or real relocation fields differ')
    return VENDOR


def verify_pow_original(original, function: dict) -> None:
    proof=function['corroboration']['runtime_pow']
    require(function['canonical_identifier']=='__CIpow' and function['size']==27 and
            function['source']=='Runtime/MSVC/pow.obj' and function['source_comparison_available'] is False and
            proof['vendor']==VENDOR,'Unsupported pow runtime record')
    context=proof['vendor_container']; original.check_hash(context)
    require(context['address']==function['address'] and context['size']==529 and
            original.section(context['address'],529)['name']=='.text','Pow context differs')
    raw=original.read(context['address'],529)
    require(raw[:2]==bytes.fromhex('eb00'),'Original pow entry jump differs')
    code=bytearray(raw[2:]); targets={}
    for offset,name,kind in VENDOR['relocations']:
        value=struct.unpack_from('<I',code,offset)[0]
        destination=(context['address']+2+offset+4+struct.unpack_from('<i',code,offset)[0])&0xffffffff if kind==20 else value
        require(name not in targets or targets[name]==destination,'Repeated pow context operand differs')
        targets[name]=destination
        if kind==20:
            require(original.section(destination,1)['name']=='.text','Pow context call leaves original text')
        else:
            owners=[s for s in original.metadata['sections'] if s['virtual_address']<=destination and
                    destination+4<=s['virtual_address']+s['virtual_size']]
            require(len(owners)==1 and owners[0]['name'] in ('.data','.rdata'),'Pow data operand has no unique owner')
        code[offset:offset+4]=bytes(4)
    require(hashlib.sha256(code).hexdigest()==VENDOR['normalized_sha256'],
            'Original pow differs outside genuine vendor relocation fields')
    require(original.read(targets['POW_name'],4)==b'pow\0','Original implementation lacks complete pow name')
    require(raw[18]==0xe8 and function['address']+23+struct.unpack_from('<i',raw,19)[0]==function['address']+36 and
            raw[23:27]==bytes.fromhex('83c414c3'),'Pow wrapper call/stack/return differs')
