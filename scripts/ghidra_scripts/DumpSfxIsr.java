// docs/98: linear disassembly of the sound-board protocol block 0x00041439..0x000415c9 (the five probe ISR stubs)
// (fn_040f4f base scan, fn_040fb3 irq probe, fn_0411d9 id, fn_04123d port init, fn_0412e9
// detect write, fn_0412a3 detect read, fn_041340 command, fn_041390 poll, fn_0413e7 reset/init).
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpSfxIsr extends GhidraScript {
    public void run() throws Exception {
        Address start = toAddr(0x41439), end = toAddr(0x415c9);
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
    }

    private void disassembleAll(Address start, Address end) {
        Address a = start;
        int n = 0;
        while (a != null && a.compareTo(end) < 0 && n++ < 4000) {
            if (getCurrentProgram().getListing().getCodeUnitAt(a) == null) {
                disassemble(a);
            }
            Instruction ins = getCurrentProgram().getListing().getInstructionAt(a);
            a = (ins != null) ? ins.getMaxAddress().next() : a.add(1);
        }
    }
}
