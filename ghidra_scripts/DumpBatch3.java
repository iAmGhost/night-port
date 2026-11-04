// Dump batch-3: frame-pipeline wrappers.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;
import ghidra.program.model.address.Address;
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import java.io.FileWriter;

public class DumpBatch3 extends GhidraScript {
    String[] names = {"pipe_22f5","pipe_209c","keypoll_47e3","pipe_20f5","stgev_3d4e4","render_8098","entloop_203d","triga_asm","trigb_asm","trigc_asm"};
    long[] addrs = {0x322f5L,0x3209cL,0x347e3L,0x320f5L,0x3d4e4L,0x38098L,0x3203dL,0x3cb38L,0x3cbd9L,0x3cc6bL};
    public void run() throws Exception {
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        FileWriter w = new FileWriter("~/Git/night-port/analysis/disasm/batch3_dump.c");
        for (int i = 0; i < names.length; i++) {
            Address a = toAddr(addrs[i]);
            Function f = getFunctionAt(a);
            String body = "(no function at 0x" + Long.toHexString(addrs[i]) + ")";
            if (f != null) {
                DecompileResults r = dec.decompileFunction(f, 30, monitor);
                if (r != null && r.decompileCompleted()) body = r.getDecompiledFunction().getC();
                if (i >= 7) {
                    // ASM appended via separate MCP disassemble calls
                }
            }
            w.write("// === " + names[i] + " @ " + String.format("%06x", addrs[i]) + " ===\n\n" + body + "\n\n\n");
            println(names[i] + " -> " + (f == null ? "NULL" : f.getName()));
        }
        w.close();
    }
}
