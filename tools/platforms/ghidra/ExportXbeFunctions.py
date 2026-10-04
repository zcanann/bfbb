# Export analyzer-derived candidate function bodies; boundaries remain heuristic.
#@category XboxAnalysis
import json
from ghidra.framework import Application
args=getScriptArgs()
metadata=json.load(open(args[0]))
section=next(s for s in metadata['sections'] if s['name']=='.text')
start=section['virtual_address'];end=start+section['raw_size']
functions=[]
listing=currentProgram.getListing()
for function in currentProgram.getFunctionManager().getFunctions(True):
    entry=function.getEntryPoint().getOffset()
    if function.isExternal() or not (start <= entry < end):
        continue
    body=function.getBody()
    ranges=[]
    for address_range in body.getAddressRanges():
        ranges.append([address_range.getMinAddress().getOffset(),address_range.getMaxAddress().getOffset()+1])
    instructions=list(listing.getInstructions(body,True))
    instruction_bytes=sum(i.getLength() for i in instructions)
    functions.append({'name':function.getName(),'entry':entry,'ranges':ranges,'body_bytes':body.getNumAddresses(),'instruction_bytes':instruction_bytes,'instruction_count':len(instructions),'thunk':function.isThunk(),'source_type':str(function.getSymbol().getSource())})
result={'executable_sha1':metadata['sha1'],'method':'Standard Ghidra x86 analysis of original section bytes, seeded by XBE entry','ghidra_version':str(Application.getApplicationVersion()),'language':str(currentProgram.getLanguageID()),'compiler_spec':str(currentProgram.getCompilerSpec().getCompilerSpecID()),'scope':'.text function entries','functions':functions}
with open(args[1],'w') as output:
    json.dump(result,output,indent=2)
print('Exported %d analyzer function candidates'%len(functions))
