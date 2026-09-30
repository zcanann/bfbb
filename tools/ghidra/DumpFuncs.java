// Dump decompiled C for a list of functions named in a file, one per line.
//
// Reading names from a file rather than argv matters on Windows: mangled C++
// names can contain '<' and '>' (e.g. xUtil_choose<i>__FPCiiPCf), which
// cmd.exe treats as redirection and which silently breaks the whole
// analyzeHeadless invocation with no error message.
//
// Also single-pass: the old version scanned every function in the program once
// per requested name.
//
// usage: -postScript DumpFuncs.java <out.c> <names.txt>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import java.io.*;
import java.util.*;

public class DumpFuncs extends GhidraScript {
    @Override
    public void run() throws Exception {
        String[] args = getScriptArgs();
        String outPath = args[0];
        String listPath = args[1];

        Set<String> want = new LinkedHashSet<>();
        BufferedReader br = new BufferedReader(new FileReader(listPath));
        for (String line; (line = br.readLine()) != null; ) {
            line = line.trim();
            if (!line.isEmpty()) want.add(line);
        }
        br.close();

        DecompInterface di = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        di.setOptions(opts);
        di.toggleCCode(true);
        di.setSimplificationStyle("decompile");
        di.openProgram(currentProgram);

        PrintWriter pw = new PrintWriter(new FileWriter(outPath));
        Set<String> found = new HashSet<>();
        int n = 0, failed = 0;

        for (Function f : currentProgram.getFunctionManager().getFunctions(true)) {
            String nm = f.getName();
            if (!want.contains(nm)) continue;
            found.add(nm);
            DecompileResults r = di.decompileFunction(f, 180, monitor);
            pw.println("// ==== " + nm + " @ " + f.getEntryPoint() + " ====");
            if (r.decompileCompleted()) {
                pw.println(r.getDecompiledFunction().getC());
            } else {
                pw.println("// FAILED: " + r.getErrorMessage());
                failed++;
            }
            pw.println();
            n++;
        }
        pw.close();

        List<String> missing = new ArrayList<>();
        for (String w : want) if (!found.contains(w)) missing.add(w);
        println("DumpFuncs wrote " + n + " functions (" + failed + " decompile failures) to " + outPath);
        println("DumpFuncs NOTFOUND " + missing.size() + (missing.isEmpty() ? "" : ": " + String.join(",", missing)));
    }
}
