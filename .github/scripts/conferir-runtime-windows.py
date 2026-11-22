"""Falha se o plugin do Windows depender do Visual C++ Redistributable (VCRUNTIME/MSVCP)."""
import glob, sys, pefile
dlls = glob.glob(sys.argv[1] + "/*.vst3/Contents/x86_64-win/*.vst3")
assert dlls, "nenhum .vst3 encontrado em " + sys.argv[1]
ruim = False
for d in dlls:
    pe = pefile.PE(d, fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
    deps = sorted(e.dll.decode() for e in getattr(pe, "DIRECTORY_ENTRY_IMPORT", []))
    vc = [x for x in deps if x.upper().startswith(("VCRUNTIME", "MSVCP", "API-MS-WIN-CRT"))]
    print(d, "->", ", ".join(deps))
    if vc:
        print("::error::o plugin depende do runtime do Visual C++:", ", ".join(vc)); ruim = True
sys.exit(1 if ruim else 0)
