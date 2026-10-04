# Map authenticated XBE sections before standard Ghidra analysis.
#@category XboxAnalysis
import json
import hashlib
from jarray import zeros
from java.io import FileInputStream
args=getScriptArgs()
metadata=json.load(open(args[0]))
memory=currentProgram.getMemory()
block=memory.getBlocks()[0]
with open(args[1],'rb') as original:
    original_bytes=original.read()
if len(original_bytes)!=metadata['size'] or hashlib.sha1(original_bytes).hexdigest()!=metadata['sha1']:
    raise ValueError('XBE input does not match analysis metadata')
text_sections=[s for s in metadata['sections'] if s['name']=='.text']
if len(text_sections)!=1:
    raise ValueError('Expected one .text section')
text=text_sections[0]
if block.getStart().getOffset()!=text['virtual_address'] or block.getSize()!=text['raw_size']:
    raise ValueError('Imported .text address/size does not match XBE metadata')
loaded_hash=hashlib.sha1()
position=0
while position<text['raw_size']:
    size=min(65536,text['raw_size']-position)
    buffer=zeros(size,'b')
    if memory.getBytes(block.getStart().add(position),buffer)!=size:
        raise ValueError('Cannot read all imported .text bytes')
    loaded_hash.update(''.join(chr(value & 255) for value in buffer))
    position+=size
if loaded_hash.hexdigest()!=text['sha1']:
    raise ValueError('Imported .text bytes do not match XBE metadata')
block.setName('.text')
block.setRead(True)
block.setWrite(False)
block.setExecute(True)
stream=FileInputStream(args[1])
try:
    filebytes=memory.createFileBytes('original.xbe',0,metadata['size'],stream,monitor)
finally:
    stream.close()
for section in metadata['sections']:
    if section['name']=='.text':
        continue
    address=toAddr(section['virtual_address'])
    if section['raw_size']:
        block=memory.createInitializedBlock(section['name'],address,filebytes,section['raw_offset'],section['raw_size'],False)
        block.setRead(True)
        block.setWrite(bool(section['flags'] & 1))
        block.setExecute(bool(section['flags'] & 4) and section['name'] not in ('.data','.rdata'))
    tail=section['virtual_size']-section['raw_size']
    if tail>0:
        block=memory.createUninitializedBlock(section['name']+'_zerofill',address.add(section['raw_size']),tail,False)
        block.setRead(True)
        block.setWrite(bool(section['flags'] & 1))
        block.setExecute(False)
entry=toAddr(metadata['entry_point'])
currentProgram.getSymbolTable().addExternalEntryPoint(entry)
disassemble(entry)
createFunction(entry,'xbe_entry')
print('Mapped XBE sections and seeded real entry point '+str(entry))
