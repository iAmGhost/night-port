// docs/92: dump every reference to the DS key bytes and decompile the WRITING functions,
// so the host's key map is grounded in the shipped code, not guessed.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class DumpKeyXrefs extends GhidraScript {
    public void run() throws Exception {
        long[] addrs = {
            0x62c75, 0x62c82, 0x62c8e, 0x62c90, 0x62cf5,
            0x63ea4, 0x63ea5, 0x63e89, 0x63eb2, 0x63ebe,
            0x63eb4, 0x63ec0, 0x63ec1, 0x63ec3, 0x62c58, 0x62cd8
        };
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        java.util.Set<Long> decompiled = new java.util.HashSet<Long>();
        for (long a : addrs) {
            Address addr = toAddr(a);
            StringBuilder sb = new StringBuilder();
            sb.append("=== DAT_").append(String.format("%08x", a)).append("\n");
            for (Reference r : currentProgram.getReferenceManager().getReferencesTo(addr)) {
                Function f = currentProgram.getFunctionManager().getFunctionContaining(r.getFromAddress());
                sb.append("  ref from ").append(r.getFromAddress()).append(" type ").append(r.getReferenceType())
                  .append(" func ").append(f == null ? "(none)" : f.getName() + " @ " + f.getEntryPoint()).append("\n");
                Instruction ins = currentProgram.getListing().getInstructionAt(r.getFromAddress());
                if (ins != null) sb.append("      insn: ").append(ins).append("\n");
                if (r.getReferenceType().isWrite() && f != null && !decompiled.contains(f.getEntryPoint().toString())) {
                    decompiled.add((long) f.getEntryPoint().toString().hashCode());
                    DecompileResults res = dec.decompileFunction(f, 60, monitor);
                    sb.append("      --- DECOMPILE ").append(f.getName()).append(" @ ").append(f.getEntryPoint()).append(" ---\n");
                    if (res.decompileCompleted()) {
                        String code = res.getDecompiledFunction().getC();
                        for (String line : code.split("\n")) sb.append("        ").append(line).append("\n");
                    } else {
                        sb.append("        failed: ").append(res.getErrorMessage()).append("\n");
                    }
                }
            }
            println(sb.toString());
        }
        dec.dispose();
    }
}
