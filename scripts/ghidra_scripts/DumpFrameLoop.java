// Decompile a list of functions into a single C file for offline analysis.
import ghidra.app.decompiler.DecompInterface;
import ghidra.app.decompiler.DecompileResults;
import java.io.FileWriter;
import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.Function;

public class DumpFrameLoop extends GhidraScript {
  static String[] P = {
    "frame","0x3a829",
    "ent_ai","0x31a8e",
    "ent_upd","0x12616",
    "ent_upd2","0x1441e",
    "in_lo","0x34735",
    "in_hi","0x34635",
    "spawner","0x3d606",
    "trig_a","0x3cb38",
    "trig_b","0x3cbd9",
    "trig_c","0x3cc6b",
    "wavereset","0x376c8",
    "stageend","0x10d47",
    "draw_ce0b","0x3ce0b",
    "draw_203d","0x3203d",
    "draw_df96","0x3df96",
    "draw_b953","0x3b953",
    "music_272f","0x3272f",
    "pre_b9b7","0x3b9b7",
    "pre_40042","0x40042",
    "pre_48e2","0x348e2",
    "pre_ed6f","0x3ed6f",
    "pre_801b","0x3801b",
    "pre_40660","0x40660",
    "pre_81d7","0x381d7",
    "sfx_4185e","0x4185e",
    "cd_d244","0x3d244",
    "entdraw_3aad","0x13aad",
    "boss_348f","0x1348f",
    "boss_37c1","0x137c1",
  };
  @Override public void run() throws Exception {
    DecompInterface dec = new DecompInterface();
    dec.openProgram(currentProgram);
    StringBuilder sb = new StringBuilder();
    for (int i = 0; i < P.length; i += 2) {
      ghidra.program.model.address.Address a = toAddr(P[i+1]);
      Function f = currentProgram.getFunctionManager().getFunctionAt(a);
      if (f == null) { sb.append("// === ").append(P[i]).append(" @ ").append(P[i+1]).append(" : NO FUNCTION ===\n\n"); continue; }
      DecompileResults dr = dec.decompileFunction(f, 60, monitor);
      if (dr.decompileCompleted()) {
        sb.append("// === ").append(P[i]).append(" @ ").append(a).append(" ===\n");
        sb.append(dr.getDecompiledFunction().getC());
      } else {
        sb.append("// === ").append(P[i]).append(" @ ").append(P[i+1]).append(" : DECOMPILE FAILED ===\n");
      }
      sb.append("\n\n");
    }
    String out = "~/Git/night-port/scratch/ghidra/frame_loop_dump.c";
    FileWriter w = new FileWriter(out);
    w.write(sb.toString());
    w.close();
    monitor.doneMsg("wrote " + sb.length() + " chars to " + out);
  }
}
