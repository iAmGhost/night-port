// docs/92: linear disassembly of the unanalyzed keyboard-handler block 0x0003fc00..0x0003fff0
// (the only WRITE xref to the DS key bytes is at 0x0003fe06), plus a decompile after creating a
// function there if possible.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpTitleWait extends GhidraScript {
    public void run() throws Exception {
        Address start = toAddr(0x38ea0), end = toAddr(0x38f40);
        disassembleAll(start, end);          // force-code the unreferenced gaps in the block
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(start, true);
        StringBuilder sb = new StringBuilder();
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(end) > 0) break;
            sb.append(ins.getAddress()).append("  ").append(ins).append("\n");
        }
        println(sb.toString());
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
