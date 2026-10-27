// docs/92: decompile the keyboard pump family and dump 0x3fb00..0x3fd40 (the block AROUND the IRQ
// handler: candidate writers of the combo latches DAT_00062c75/0x62c82/0x62c8e/0x62c90/0x62cf5 and
// of DAT_00063eb4).
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.*;

public class DumpKbdFamily extends GhidraScript {
    public void run() throws Exception {
        DecompInterface dec = new DecompInterface();
        dec.openProgram(currentProgram);
        long[] entries = {0x34353, 0x343d1, 0x3442e, 0x34635, 0x34735, 0x347e3, 0x348e2, 0x3493e};
        for (long e : entries) {
            Function f = currentProgram.getFunctionManager().getFunctionAt(toAddr(e));
            StringBuilder sb = new StringBuilder();
            sb.append("=== fn_").append(String.format("%06x", e)).append("\n");
            if (f == null) { sb.append("  (no function)\n"); }
            else {
                DecompileResults res = dec.decompileFunction(f, 120, monitor);
                if (res.decompileCompleted()) sb.append(res.getDecompiledFunction().getC());
                else sb.append("  failed: ").append(res.getErrorMessage()).append("\n");
            }
            println(sb.toString());
        }
        // linear disassembly of the surrounding block
        Address a = toAddr(0x3fb00), end = toAddr(0x3fd40);
        int n = 0;
        while (a != null && a.compareTo(end) < 0 && n++ < 3000) {
            if (currentProgram.getListing().getCodeUnitAt(a) == null) disassemble(a);
            a = a.next();
        }
        StringBuilder sb = new StringBuilder("=== DIS 0x3fb00..0x3fd40\n");
        InstructionIterator it = currentProgram.getListing().getInstructions(toAddr(0x3fb00), true);
        while (it.hasNext()) {
            Instruction ins = it.next();
            if (ins.getAddress().compareTo(end) > 0) break;
            sb.append(ins.getAddress()).append("  ").append(ins).append("\n");
        }
        println(sb.toString());
        dec.dispose();
    }
}
