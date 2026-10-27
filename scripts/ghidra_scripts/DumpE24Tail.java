// docs/98: find the producer of DAT_00053e24 (ten device command-stream cursors).
// 1) xrefs to 0x53e24; 2) linear disasm of 0x40000..0x42600 hunting writes through
// base+displacement forms mentioning 53e24/53e48 or via a register base.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;

public class DumpE24Tail2 extends GhidraScript {
    public void run() throws Exception {
        for (long d = 0x53e24; d <= 0x53e48; d += 4) {
            Address a = toAddr(d);
            Reference[] r = getReferencesTo(a);
            if (r.length > 0) {
                for (Reference x : r) println("XREF to " + a + " from " + x.getFromAddress() + " " + x.getReferenceType());
            }
        }
        // disassemble the whole code body, then collect any instruction whose text mentions e24/e48 dat
        Address start = toAddr(0x0), end = toAddr(0x46000);
        Address a = start;
        int n = 0;
        while (a != null && a.compareTo(end) < 0 && n++ < 200000) {
            if (getCurrentProgram().getListing().getCodeUnitAt(a) == null) disassemble(a);
            Instruction ins = getCurrentProgram().getListing().getInstructionAt(a);
            a = (ins != null) ? ins.getMaxAddress().next() : a.add(1);
        }
        Listing listing = currentProgram.getListing();
        InstructionIterator it = listing.getInstructions(true);
        StringBuilder sb = new StringBuilder();
        while (it.hasNext()) {
            Instruction ins = it.next();
            String s = ins.toString();
            if (s.contains("53e") || s.contains("53d0c")) {
                sb.append(ins.getAddress()).append("  ").append(s).append("\n");
            }
        }
        println("MATCHES:\n" + sb);
    }
}
