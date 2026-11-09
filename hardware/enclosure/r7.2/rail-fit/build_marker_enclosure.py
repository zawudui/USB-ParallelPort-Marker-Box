"""Compatiblity adapter for the archived C snap-fit generator.

The original generator imports a "rail-fit" helper not bundled with the
original public release. Reuse the included first-fit reference script and
the frozen C-design locating-hole data recorded in params.json.
"""
from pathlib import Path
import importlib.util
import json

HERE = Path(__file__).resolve().parent
reference = HERE.parent / "first-fit" / "build_marker_enclosure.py"
spec = importlib.util.spec_from_file_location("first_fit_enclosure_reference", reference)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
for name in ("connector_spec", "normalize_step", "USB_WINDOW", "DB25_WINDOW", "PAD_C", "FIN_CY"):
    globals()[name] = getattr(module, name)

parameters = json.loads((HERE.parent / "snap-fit-pa12" / "params.json").read_text(encoding="utf-8"))
RAIL_HOLES = tuple(tuple(x) for x in parameters["rail_holes_xy_mm"])
RAIL_HOLE_D = parameters["rail_hole_diameter_mm"]
RAIL_HOLE_REFERENCE = parameters["rail_hole_reference"]
