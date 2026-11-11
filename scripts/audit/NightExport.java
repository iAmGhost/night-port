import ghidra.app.script.GhidraScript;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.*;
import ghidra.program.model.data.*;
import ghidra.program.model.address.*;
import java.io.*;
import java.util.*;

public class NightExport extends GhidraScript {
  static String e(String s) {
    if (s == null) return "";
    StringBuilder b = new StringBuilder();
    for (int i = 0; i < s.length(); i++) {
      char c = s.charAt(i);
      if (c == '"') b.append("\\\"");
      else if (c == '\\') b.append("\\\\");
      else if (c == '\n') b.append("\\n");
      else if (c < 0x20) b.append(" ");
      else b.append(c);
    }
    return b.toString();
  }

  static String hex(Address a) {
    return a == null ? "" : "0x" + Long.toHexString(a.getOffset());
  }

  public void run() throws Exception {
    Program p = currentProgram;
    File dir = new File(repo(), "scripts/audit");
    dir.mkdirs();

    // ---------------- symbols ----------------
    PrintWriter w = new PrintWriter(new BufferedWriter(new FileWriter(new File(dir, "symbols.json"))));
    w.print("[");
    int nsym = 0, ndata = 0;
    SymbolIterator sit = p.getSymbolTable().getSymbolIterator(false);
    while (sit.hasNext()) {
      Symbol s = sit.next();
      Address a = s.getAddress();
      if (!a.getAddressSpace().getName().equals("ram")) continue;
      Data dd = p.getListing().getDefinedDataAt(a);
      if (nsym++ > 0) w.print(",");
      w.print("{");
      w.print("\"name\":\"" + e(s.getName()) + "\",\"addr\":\"" + hex(a) + "\"");
      w.print(",\"primary\":" + (s.isPrimary() ? "true" : "false"));
      w.print(",\"stype\":\"" + e(s.getSymbolType().toString()) + "\"");
      w.print(",\"refcount\":" + s.getReferenceCount());
      if (dd != null) {
        ndata++;
        w.print(",\"size\":" + dd.getLength());
        w.print(",\"type\":\"" + e(String.valueOf(dd.getDataType())) + "\"");
      }
      w.print("}");
      if (monitor.isCancelled()) break;
    }
    w.print("]");
    w.close();

    // ---------------- functions + call edges ----------------
    w = new PrintWriter(new BufferedWriter(new FileWriter(new File(dir, "functions.json"))));
    w.print("[");
    int nfn = 0;
    FunctionIterator fit = p.getFunctionManager().getFunctions(true);
    while (fit.hasNext()) {
      Function f = fit.next();
      if (!f.getEntryPoint().getAddressSpace().getName().equals("ram")) continue;
      Set<Address> callees = new TreeSet<Address>();
      InstructionIterator iit = p.getListing().getInstructions(f.getBody(), true);
      while (iit.hasNext()) {
        Instruction ins = iit.next();
        if (ins.getFlowType().isCall()) {
          for (Reference r : ins.getReferencesFrom()) {
            if (r.getReferenceType().isCall()) callees.add(r.getToAddress());
          }
        }
      }
      if (nfn++ > 0) w.print(",");
      w.print("{");
      w.print("\"name\":\"" + e(f.getName()) + "\",\"entry\":\"" + hex(f.getEntryPoint()) + "\"");
      w.print(",\"body_start\":\"" + hex(f.getBody().getMinAddress()) + "\",\"body_end\":\"" + hex(f.getBody().getMaxAddress()) + "\"");
      w.print(",\"body_bytes\":" + f.getBody().getNumAddresses());
      w.print(",\"sig\":\"" + e(f.getSignature().toString()) + "\"");
      w.print(",\"thunk\":" + (f.isThunk() ? "true" : "false"));
      StringBuilder cs = new StringBuilder();
      int k = 0;
      for (Address c : callees) {
        if (k++ > 0) cs.append(",");
        cs.append("\"" + hex(c) + "\"");
      }
      w.print(",\"callees\":[" + cs.toString() + "]");
      w.print("}");
      if (monitor.isCancelled()) break;
    }
    w.print("]");
    w.close();

    // ---------------- every instruction's DS-region memory access, with operand text ----------------
    w = new PrintWriter(new BufferedWriter(new FileWriter(new File(dir, "mem_access.json"))));
    w.print("[");
    int nm = 0;
    InstructionIterator iit = p.getListing().getInstructions(true);
    while (iit.hasNext()) {
      Instruction ins = iit.next();
      Reference[] rs = ins.getReferencesFrom();
      if (rs.length == 0) continue;
      boolean ds = false;
      for (Reference r : rs) {
        if (r.getToAddress().getAddressSpace().getName().equals("ram")
            && r.getToAddress().getOffset() >= 0x50000 && r.getToAddress().getOffset() <= 0x66000) ds = true;
      }
      if (!ds) continue;
      Function f = p.getFunctionManager().getFunctionContaining(ins.getMinAddress());
      StringBuilder toks = new StringBuilder();
      int k = 0;
      for (int i = 0; i < ins.getNumOperands(); i++) {
        if (k++ > 0) toks.append(",");
        toks.append("\"" + e(ins.getDefaultOperandRepresentation(i)) + "\"");
      }
      StringBuilder rl = new StringBuilder();
      int q = 0;
      for (Reference r : rs) {
        if (q++ > 0) rl.append(",");
        Data rd = p.getListing().getDefinedDataAt(r.getToAddress());
        rl.append("{\"to\":\"" + hex(r.getToAddress()) + "\",\"rtype\":\"" + e(r.getReferenceType().toString())
            + "\",\"op\":" + r.getOperandIndex()
            + ",\"sym\":\"" + e(rd == null ? "" : rd.getLabel()) + "\"}");
      }
      if (nm++ > 0) w.print(",");
      w.print("{");
      w.print("\"addr\":\"" + hex(ins.getMinAddress()) + "\"");
      w.print(",\"fn\":\"" + e(f == null ? "" : f.getName()) + "\"");
      w.print(",\"mnem\":\"" + e(ins.getMnemonicString()) + "\"");
      w.print(",\"ops\":[" + toks.toString() + "]");
      w.print(",\"refs\":[" + rl.toString() + "]");
      w.print(",\"flow\":\"" + e(ins.getFlowType().toString()) + "\"");
      w.print("}");
      if (monitor.isCancelled()) break;
    }
    w.print("]");
    w.close();

    // ---------------- all defined data (labels, sizes, types) ----------------
    w = new PrintWriter(new BufferedWriter(new FileWriter(new File(dir, "defined_data.json"))));
    w.print("[");
    int ndall = 0;
    DataIterator dit = p.getListing().getDefinedData(true);
    while (dit.hasNext()) {
      Data d = dit.next();
      Address a = d.getMinAddress();
      if (!a.getAddressSpace().getName().equals("ram")) continue;
      if (ndall++ > 0) w.print(",");
      w.print("{");
      w.print("\"addr\":\"" + hex(a) + "\",\"end\":\"" + hex(d.getMaxAddress()) + "\"");
      w.print(",\"size\":" + d.getLength());
      w.print(",\"type\":\"" + e(String.valueOf(d.getDataType())) + "\"");
      w.print(",\"label\":\"" + e(d.getLabel()) + "\"");
      w.print(",\"nrefs\":" + d.getValueReferences().length);
      w.print("}");
      if (monitor.isCancelled()) break;
    }
    w.print("]");
    w.close();

    PrintWriter info = new PrintWriter(new File(dir, "export_info.json"));
    info.print("{\"program\":\"" + e(p.getName()) + "\",\"exe\":\"" + e(p.getExecutablePath()) + "\","
        + "\"symbols_exported\":" + nsym + ",\"defined_data\":" + ndata + ",\"defined_data_all\":" + ndall + ",\"functions\":" + nfn
        + ",\"data_xrefs\":" + 0 + ",\"mem_access\":" + nm
        + ",\"ghidra_functions\":" + p.getFunctionManager().getFunctionCount()
        + ",\"ghidra_symbols\":" + p.getSymbolTable().getNumSymbols()
        + ",\"ghidra_defined_data\":" + p.getListing().getNumDefinedData()
        + ",\"image_base\":\"" + hex(p.getImageBase()) + "\"}");
    info.close();
    println("EXPORT DONE syms=" + nsym + " data=" + ndata + " fns=" + nfn + " mem=" + nm);
  }

    /* The repo root is resolved, never hard-coded: NIGHT_REPO names it, and a headless run started from
     * the repo root needs nothing. Nothing tracked here gets to know which machine it was written on. */
    static File repo() {
        String e = System.getenv("NIGHT_REPO");
        return new File(e != null && !e.isEmpty() ? e : System.getProperty("user.dir"));
    }

}
