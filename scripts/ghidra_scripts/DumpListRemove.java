// docs/95: disassembly + decompile of fn_0003b67e (0x0003b67e, 153B) list-remove, to settle the
// null-successor write semantics on the last node.
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpListRemove extends GhidraScript {
    public void run() throws Exception {
        Address start = toAddr(0x3b67e), end = toAddr(0x3b6d9);
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(start, true);
        StringBuilder sb = new StringBuilder();
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(end) > 0) break;
            sb.append(ins.getAddress()).append("  ").append(ins).append("\n");
        }
        println("== DISASM 3b67e ==\n" + sb);
        DecompInterface d = new DecompInterface();
        d.openProgram(currentProgram);
        DecompileResults r = d.decompileFunction(currentProgram.getFunctionManager().getFunctionAt(start), 60, monitor);
        if (r != null && r.decompileCompleted()) println("== DECOMP ==\n" + r.getDecompiledFunction().getC());
    }
}
