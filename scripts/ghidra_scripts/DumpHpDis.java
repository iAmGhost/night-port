// docs/94: linear disassembly of the fn_01441e damage/HP block 0x0001488a..0x000149c0 to pin the
// write widths of the two-word HP model (+0x23 debt word, +0x29 hp word, +0x56 overshoot dword).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.app.decompiler.*;
import ghidra.program.model.symbol.*;

public class DumpHpDis extends GhidraScript {
    public void run() throws Exception {
        Address start = toAddr(0x1488a), end = toAddr(0x149c0);
        disassembleAll(start, end);
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(start, true);
        StringBuilder sb = new StringBuilder();
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(end) > 0) break;
            sb.append(ins.getAddress()).append("  ").append(ins).append("\n");
        }
        println(sb.toString());
        DecompInterface decomp = new DecompInterface();
        decomp.openProgram(currentProgram);
        Function f = currentProgram.getFunctionManager().getFunctionAt(toAddr(0x1441e));
        if (f != null) {
            DecompileResults r = decomp.decompileFunction(f, 120, monitor);
            if (r.decompileCompleted())
                println("DECOMPiled:\n" + r.getDecompiledFunction().getC());
        }
    }

    private void disassembleAll(Address start, Address end) {
        Address a = start;
        int n = 0;
        while (a != null && a.compareTo(end) < 0 && n++ < 4000) {
            if (getCurrentProgram().getListing().getCodeUnitAt(a) == null) {
                disassemble(a);
            }
            Instruction ins = getCurrentProgram().getListing().getInstructionAt(a);
            a = (ins != null) ? ins.getMaxAddress().next() : a.next();
        }
    }
}
