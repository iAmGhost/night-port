// Phase-0 inventory dump (Ghidra 12 API):
//  - analysis/pseudo/NNN_<name>_<addr>.c   decompilation of every function
//  - analysis/functions.tsv                idx, start, end, size, name, thunk, external
//  - analysis/callgraph.tsv                caller -> callee (all call references)
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import ghidra.app.script.GhidraScript;
import ghidra.program.database.function.FunctionDB;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionIterator;
import ghidra.program.model.listing.Program;
import ghidra.program.model.symbol.Reference;
import java.io.File;
import java.io.FileWriter;
import java.util.ArrayList;

public class DumpAll extends GhidraScript {
    public void run() throws Exception {
        Program prog = currentProgram;
        String root = "~/Git/night-port/analysis";
        File pseudoDir = new File(root, "pseudo");
        pseudoDir.mkdirs();

        ArrayList<Function> list = new ArrayList<>();
        for (FunctionIterator it2 = prog.getFunctionManager().getFunctions(true); it2.hasNext(); ) {
            list.add(it2.next());
        }
        int n = list.size();
        Function[] fns = list.toArray(new Function[0]);
        java.util.Arrays.sort(fns, (a, b) -> a.getEntryPoint().compareTo(b.getEntryPoint()));

        int ok = 0, fail = 0;
        FileWriter tsv = new FileWriter(root + "/functions.tsv");
        tsv.write("idx\tstart\tend\tsize\tname\tthunk\texternal\n");

        DecompInterface dec = new DecompInterface();
        dec.openProgram(prog);

        for (int i = 0; i < n; i++) {
            Function f = fns[i];
            Address a = f.getEntryPoint();
            String hex = String.format("%06x", a.getOffset());
            FunctionDB fd = (FunctionDB) f;
            long size = fd.getBody().getNumAddresses();
            String name = (f.getName() == null ? "FUN" : f.getName()) + "_" + hex;
            tsv.write(String.format("%03d\t000%s\t000%s\t%d\t%s\t%s\t%s%n",
                i, hex, String.format("%06x", fd.getBody().getMaxAddress().getOffset()),
                size, f.getName(), f.isThunk(), f.isExternal()));

            String body = "// (no decompilation)";
            DecompileResults r = dec.decompileFunction(f, 60, monitor);
            if (r != null && r.decompileCompleted()) {
                body = r.getDecompiledFunction().getC();
                ok++;
            } else {
                fail++;
            }

            try (FileWriter w = new FileWriter(new File(pseudoDir, String.format("%03d_%s.c", i, name)))) {
                w.write("// func #" + i + "  " + f.getName() + "  " + a + "  size=" + size
                      + (f.isThunk() ? "  [thunk]" : "") + "\n\n" + body + "\n");
            }
        }
        tsv.close();

        // call graph via ReferenceManager
        FileWriter cg = new FileWriter(root + "/callgraph.tsv");
        cg.write("caller\tcallee\n");
        for (int i = 0; i < n; i++) {
            Function f = fns[i];
            FunctionDB fd = (FunctionDB) f;
            for (ghidra.program.model.address.AddressIterator ai = fd.getBody().getAddresses(true); ai.hasNext(); ) {
                Address from = ai.next();
                Reference[] refs = prog.getReferenceManager().getReferencesFrom(from);
                for (Reference ref : refs) {
                    if (!ref.getReferenceType().isCall()) continue;
                    Address to = ref.getToAddress();
                    Function cf = prog.getFunctionManager().getFunctionAt(to);
                    if (cf == null) cf = prog.getFunctionManager().getFunctionContaining(to);
                    cg.write(String.format("%06x\t%s\t%06x%n", f.getEntryPoint().getOffset(),
                            cf != null ? cf.getName() : "???", to.getOffset()));
                }
            }
        }
        cg.close();
        println("dumped " + n + " functions, decompile ok=" + ok + " fail=" + fail);
    }
}
