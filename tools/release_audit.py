#!/usr/bin/env python3
"""Read-only audit; only checks source integrity."""
from __future__ import annotations

import hashlib
import json
import re
import sys
import zipfile
from pathlib import Path
from urllib.parse import unquote

if hasattr(sys.stdout, "reconfigure"):
    sys.stdout.reconfigure(encoding="utf-8")

ROOT = Path(__file__).resolve().parents[1]
ENC = ROOT / "hardware/enclosure/r7.2/snap-fit-pa12"
FAB = ROOT / "hardware/r7.2/manufacturing"
failures = []
checks = 0


def check(ok, label):
    global checks
    checks += 1
    if not ok:
        failures.append(label)
        print("FAIL:", label)


def sha(path):
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def manifest(parent, path):
    count = 0
    for line in path.read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        m = re.fullmatch(r"([0-9a-f]{64})\s+(.+)", line)
        check(m is not None, "malformed checksum: " + str(path))
        if not m:
            continue
        target = (parent / m.group(2)).resolve()
        check(target.is_relative_to(parent.resolve()), "unsafe manifest path: " + m.group(2))
        if not target.is_relative_to(parent.resolve()):
            continue
        actual = sha(target) if target.is_file() else "MISSING"
        if actual != m.group(1):
            print("HASH DETAIL:", str(target.relative_to(ROOT)), "expected", m.group(1), "actual", actual)
        check(actual == m.group(1), "SHA-256 " + str(target.relative_to(ROOT)))
        count += 1
    check(count > 10, "checksum manifest has fewer than 11 entries: " + str(path))


def essential():
    paths = [
        "LICENSE", "LICENSE-HARDWARE", "LICENSE-FIRMWARE", "LICENSE-DOCS",
        "THIRD_PARTY_NOTICES.md", "README.md", "README_CN.md",
        "third_party/KICAD-LIBRARIES-LICENSE.md",
        "docs/release/RELEASE_GATE.md", "docs/release/USB_ID_PLAN.md",
        "firmware/core/usb_ids.h", "firmware/target/stm32f411/CMakeLists.txt",
        "hardware/r7.2/kicad/usb_db25_marker.kicad_sch",
        "hardware/r7.2/kicad/usb_db25_marker.kicad_pcb",
        "hardware/r7.2/kicad/MarkerBox.kicad_sym",
        "hardware/r7.2/kicad/MarkerBox.pretty/DSUB-25_Socket_NorComp_182-025-213R531.kicad_mod",
        "hardware/r7.2/kicad/MarkerBox.pretty/TMR2_0511_SIP8.kicad_mod",
        "hardware/enclosure/r7.2/snap-fit-pa12/base_r72_snap.step",
        "hardware/enclosure/r7.2/snap-fit-pa12/lid_r72_snap.step",
        "hardware/enclosure/r7.2/snap-fit-pa12/assembled_r72_snap.step",
        "hardware/enclosure/r7.2/snap-fit-pa12/build_snap_enclosure.py",
        "hardware/enclosure/r7.2/rail-fit/build_marker_enclosure.py",
        "hardware/enclosure/r7.2/snap-fit-pa12/print-package-two-pa12-parts.zip",
        "hardware/r7.2/manufacturing/usb_db25_marker_r7.2_JLC_RS274X_upload.zip",
        "tools/matlab/MarkerBox.m",
        "hardware/enclosure/r7.2/snap-fit-pa12/lid_r72_snap.stl",
        ".gitattributes", "SOURCE_SHA256SUMS",
        "third_party/licenses/KiCad-LIBRARIES-LICENSE.txt",
        "third_party/licenses/CC-BY-SA-4.0.txt",
        "third_party/licenses/CC-BY-4.0.txt",
    ]
    for item in paths:
        check((ROOT / item).is_file(), "essential file: " + item)


def enclosure():
    spec = json.loads((ENC / "params.json").read_text(encoding="utf-8"))
    record = json.loads((ENC / "mesh_verification.json").read_text(encoding="utf-8"))
    check(spec["printed_part_count"] == 2 and spec["snap_count"] == 4, "2 parts / 4 snaps")
    check(len(spec["rail_holes_xy_mm"]) == 4, "four PCB locating holes")
    check(record["source_sha256"] == sha(ENC / "build_snap_enclosure.py"), "CAD generator record")
    for kind in ("base", "lid"):
        part = next(x for x in record["parts"] if x["part"] == kind)
        check(sha(ENC / (kind + "_r72_snap.step")) == part["step_sha256"], kind + " STEP archive record")
    with zipfile.ZipFile(ENC / "print-package-two-pa12-parts.zip") as z:
        items = [x for x in z.namelist() if not x.endswith("/")]
        names = [Path(x).name for x in items]
        print("ZIP CONTENTS:", items)
        check(len(items) == 3 and len(set(names)) == 3 and set(names) == {"base_r72_snap.stl", "lid_r72_snap.stl", "打印要求.txt"}, "2 STL parts plus print instructions")
        for member in items:
            if Path(member).suffix.lower() != ".stl":
                continue
            check(sha(ENC / Path(member).name) == hashlib.sha256(z.read(member)).hexdigest(), "standalone STL matches ZIP: " + member)
            kind = Path(member).name.split("_")[0]
            matches = [x for x in record["parts"] if x["part"] == kind]
            if not matches:
                check(False, "unknown STL archive member: " + member)
                continue
            check(hashlib.sha256(z.read(member)).hexdigest() == matches[0]["stl_sha256"], "STL archive: " + member)


def links():
    found = 0
    for doc in ROOT.rglob("*.md"):
        for raw in re.findall(r"\[[^\]]*\]\(([^)]+)\)", doc.read_text(encoding="utf-8")):
            target = unquote(raw.strip().split("#", 1)[0])
            if not target or target.startswith(("http:", "https:", "mailto:")):
                continue
            found += 1
            p = (doc.parent / target).resolve()
            check(p.is_relative_to(ROOT) and p.exists(), "Markdown link " + str(doc.relative_to(ROOT)) + " -> " + target)
    check(found > 10, "at least 10 local Markdown links checked")


def usb_status():
    header = (ROOT / "firmware/core/usb_ids.h").read_text(encoding="utf-8")
    has_st_ids = "0x0483u" in header and "0x5740u" in header
    check(not has_st_ids or "MKR_USB_IDS_ARE_PLACEHOLDER 1" in header, "ST sample IDs marked as placeholders")


def source_files():
    """Do not allow unnoticed private files into a first source upload."""
    lines = (ROOT / "SOURCE_SHA256SUMS").read_text(encoding="utf-8").splitlines()
    expected = {line.split("  ", 1)[1] for line in lines if line and not line.startswith("#")}
    expected.add("SOURCE_SHA256SUMS")
    for path in ROOT.rglob("*"):
        relative = path.relative_to(ROOT)
        if any(part in {".git", "__pycache__", "output", "build", "build-target", "dist", ".venv"} for part in relative.parts):
            continue
        if path.is_file():
            check(relative.as_posix() in expected, "unlisted source file: " + relative.as_posix())


def main():
    essential()
    manifest(ENC, ENC / "SHA256SUMS.txt")
    manifest(FAB, FAB / "SHA256SUMS")
    manifest(ROOT, ROOT / "SOURCE_SHA256SUMS")
    enclosure()
    links()
    usb_status()
    source_files()
    print("Source integrity:", checks - len(failures), "passed,", len(failures), "failed.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
