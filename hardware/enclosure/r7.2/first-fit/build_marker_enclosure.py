"""USB-DB25 Marker r7.2 first-fit 3D-printed enclosure.

Units: mm. Same coordinates as the EasyEDA STEP preview:
    PCB outer edges x 0..155, y -118..0; PCB lower surface z=0, upper z=1.6.

The uploaded STEP has the board and parts, but no DB25 solid. The DB25
envelope is the KiCad courtyard plus a margin, checked against the r7.1
Excellon drill and the r7.2 placement. Prototype only; a physical test-fit
is still required.

    python build_marker_enclosure.py --check
    python build_marker_enclosure.py --update-records
"""
import argparse
import hashlib
import json
import os
import re
import sys
import zipfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
OUT = os.environ.get('MARKER_BOX_OUTPUT', os.path.join(HERE, 'output'))

# Board and shell. Screw towers stay outside the board in X.
PCB_X = (0.0, 155.0)
PCB_Y = (-118.0, 0.0)
PCB_Z = (0.0, 1.6)
EXT_X = (-8.0, 163.0)
EXT_Y = (-124.0, 6.0)
BASE_Z = (-10.0, 4.5)
LID_Z = (4.5, 23.0)
WALL = 2.8
FLOOR_TOP = -7.2
ROOF_UNDER = 20.2
OUTER_R = 5.0
INNER_R = 2.2
SCREW_X = (-4.0, 159.0)
SCREW_Y = (-15.0, -110.0)
SCREW_OD = 7.0
SCREW_CLEAR = 3.4
SCREW_PILOT = 2.5
FIN_CY = (-22.0, -110.0)
FIN_HALF_Y = 4.5
RIGHT_FIN_X = (156.05, 158.55)
PAD_C = ((2.6, -22.0), (2.6, -110.0), (152.4, -22.0), (152.4, -110.0))
PAD_HALF_X = 4.0
PAD_HALF_Y = 5.0
RIB_X = (4.0, 138.0)

# J2 at KiCad (144.46, 58), rotation +90. Enclosure Y is the drill Y.
J2_X = 144.46
J2_Y = -58.0
# NorComp 182-YYY-213RYY1 rev 18, via the footprint description.
ROW_PITCH = 2.77
ROW_GAP = 2.84
FLANGE_LOCAL_Y = 10.54
SHELL_FRONT_LOCAL_Y = 17.11
SHELL_ABOVE_BOARD = 12.55
COURTYARD_LOCAL_X = (-43.67, 10.43)
COURTYARD_LOCAL_Y = (-2.31, 17.61)
FAB_LOCAL_X = (-43.17, 9.93)
FAB_REAR_LOCAL_Y = -1.8
# Flange ears in F.Fab. Their X centers match the board-lock pads.
JACK_LOCAL = ((-40.14, 13.44), (6.90, 13.44))
JACK_RADIUS = 4.0
ENVELOPE_MARGIN = 0.40
PIN_TAIL_KEEP_OUT = 4.0

# J1 mating face measured from the uploaded STEP. The cut has to reach it.
J1_FACE_X = 0.08
USB_WINDOW = dict(xa=-9.2, xb=1.2, ya=-53.0, yb=-37.0, za=-1.5, zb=8.5)
DB25_WINDOW = dict(xa=158.0, xb=164.0, ya=-103.6, yb=-45.6, za=-5.2, zb=18.0)

DRILL_ZIP_REL = 'hardware/r7.2/manufacturing/usb_db25_marker_r7.2_JLC_RS274X_upload.zip'
DRILL_MEMBER = 'usb_db25_marker-PTH.drl'
DRILL_REL = f'{DRILL_ZIP_REL}:{DRILL_MEMBER}'
PCB_REL = 'hardware/r7.2/kicad/usb_db25_marker.kicad_pcb'
FP_R72 = 'hardware/r7.2/kicad/MarkerBox.pretty/DSUB-25_Socket_NorComp_182-025-213R531.kicad_mod'
FP_EXPECTED_SHA256 = 'a924f7e29efdb4729d1b9bb56a11559a2cca478159db07d5268abd8beae863bd'


def rnd(value):
    return float(f'{value:.2f}')


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def sha256_file(path):
    digest = hashlib.sha256()
    with open(path, 'rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def enc_from_local(local_x, local_y):
    """KiCad +90° into this drill frame: local +Y moves toward the right edge."""
    return rnd(J2_X + local_y), rnd(J2_Y + local_x)


def _holes(text):
    found = []
    for line in text.splitlines():
        match = re.fullmatch(r'X([0-9.]+)Y(-?[0-9.]+)', line.strip())
        if match:
            found.append((float(match.group(1)), float(match.group(2))))
    return found


def _has_hole(holes, x, y, tol=0.02):
    return any(abs(hx - x) <= tol and abs(hy - y) <= tol for hx, hy in holes)


def connector_spec():
    """Lock the outward DB25 placement to the drill, the footprint, and the PCB."""
    drill_zip_path = os.path.join(ROOT, DRILL_ZIP_REL)
    pcb_path = os.path.join(ROOT, PCB_REL)
    fp_r72 = os.path.join(ROOT, FP_R72)
    for path in (drill_zip_path, pcb_path, fp_r72):
        require(os.path.isfile(path), 'missing ' + path)

    fp_hash = sha256_file(fp_r72)
    require(fp_hash == FP_EXPECTED_SHA256, 'r7.2 DB25 footprint SHA-256 mismatch')

    with zipfile.ZipFile(drill_zip_path) as archive:
        drill_text = archive.read(DRILL_MEMBER).decode('utf-8')
    date_match = re.search(r'TF\.CreationDate,(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2})', drill_text)
    require(date_match, 'drill file has no creation date')
    holes = _holes(drill_text)
    row1 = [hole for hole in holes if abs(hole[0] - J2_X) <= 0.02 and -91.30 <= hole[1] <= -57.90]
    row2 = [hole for hole in holes if abs(hole[0] - (J2_X + ROW_GAP)) <= 0.02 and -90.00 <= hole[1] <= -59.20]
    require(len(row1) == 13, f'expected 13 holes on X={J2_X}, found {len(row1)}')
    require(len(row2) == 12, f'expected 12 holes on the outer row, found {len(row2)}')
    require(_has_hole(holes, J2_X, J2_Y), 'pin 1 is not at X144.46 Y-58')
    near_x, near_y = enc_from_local(-1.385, ROW_GAP)
    require(_has_hole(holes, near_x, near_y), 'outer signal row is not at X147.30 Y-59.385')
    for local_x, _local_y in JACK_LOCAL:
        lock_x, lock_y = enc_from_local(local_x, 1.42)
        require(_has_hole(holes, lock_x, lock_y), f'board lock missing at {lock_x}, {lock_y}')

    pcb_text = open(pcb_path, encoding='utf-8').read()
    marker = 'MarkerBox:DSUB-25_Socket_NorComp_182-025-213R531'
    at_pos = pcb_text.find(marker)
    require(at_pos >= 0, 'r7.2 PCB has no NorComp DB25 footprint')
    placed = re.search(r'\(at\s+([0-9.]+)\s+([0-9.]+)\s+([0-9.]+)\)', pcb_text[at_pos:at_pos + 400])
    require(placed, 'r7.2 J2 has no placement')
    px, py, prot = (float(item) for item in placed.groups())
    require(abs(px - J2_X) < 1e-6 and abs(py - 58.0) < 1e-6 and abs(prot - 90.0) < 1e-6,
            f'r7.2 J2 moved to {(px, py, prot)}')

    flange_x, _ = enc_from_local(0.0, FLANGE_LOCAL_Y)
    shell_front_x, _ = enc_from_local(0.0, SHELL_FRONT_LOCAL_Y)
    row2_x, _ = enc_from_local(0.0, ROW_GAP)
    require(flange_x == PCB_X[1], 'flange is not on the right board edge')
    require(rnd(PCB_X[1] - row2_x) == 7.70, 'outer row is not 7.70 mm from the edge')
    require(shell_front_x > PCB_X[1], 'shell front is not outside the board')

    # local X becomes enclosure Y and local Y becomes enclosure X, so sort each axis.
    cr_xs = [enc_from_local(lx, ly)[0] for lx in COURTYARD_LOCAL_X for ly in COURTYARD_LOCAL_Y]
    cr_ys = [enc_from_local(lx, ly)[1] for lx in COURTYARD_LOCAL_X for ly in COURTYARD_LOCAL_Y]
    cr_xa, cr_xb = min(cr_xs), max(cr_xs)
    cr_ya, cr_yb = min(cr_ys), max(cr_ys)
    envelope = dict(
        xa=rnd(cr_xa - ENVELOPE_MARGIN),
        xb=rnd(cr_xb + ENVELOPE_MARGIN),
        ya=rnd(cr_ya - ENVELOPE_MARGIN),
        yb=rnd(cr_yb + ENVELOPE_MARGIN),
        za=rnd(-PIN_TAIL_KEEP_OUT),
        zb=rnd(PCB_Z[1] + SHELL_ABOVE_BOARD + ENVELOPE_MARGIN),
    )
    fab_xs = [enc_from_local(lx, ly)[0] for lx in FAB_LOCAL_X for ly in (FAB_REAR_LOCAL_Y, SHELL_FRONT_LOCAL_Y)]
    fab_ys = [enc_from_local(lx, ly)[1] for lx in FAB_LOCAL_X for ly in (FAB_REAR_LOCAL_Y, SHELL_FRONT_LOCAL_Y)]
    require(envelope['xa'] <= min(fab_xs) and envelope['xb'] >= max(fab_xs), 'envelope misses the shell in X')
    require(envelope['ya'] <= min(fab_ys) and envelope['yb'] >= max(fab_ys), 'envelope misses the shell in Y')
    require(envelope['xa'] <= cr_xa and envelope['xb'] >= cr_xb, 'envelope misses the courtyard in X')
    require(envelope['ya'] <= cr_ya and envelope['yb'] >= cr_yb, 'envelope misses the courtyard in Y')
    require(envelope['za'] > FLOOR_TOP, 'pin keep-out goes through the floor')
    require(envelope['zb'] < ROOF_UNDER, 'shell keep-out goes through the roof')

    inner_wall = rnd(EXT_X[1] - WALL)
    require(DB25_WINDOW['xa'] <= inner_wall, 'DB25 window does not reach the inner wall')
    require(DB25_WINDOW['xb'] >= EXT_X[1], 'DB25 window does not reach the outer face')
    require(DB25_WINDOW['ya'] <= envelope['ya'] and DB25_WINDOW['yb'] >= envelope['yb'],
            'DB25 window is narrower than the connector')
    require(DB25_WINDOW['za'] <= envelope['za'] and DB25_WINDOW['zb'] >= envelope['zb'],
            'DB25 window is shorter than the connector')
    require(DB25_WINDOW['xa'] <= shell_front_x <= DB25_WINDOW['xb'], 'shell front is outside the window')
    require(USB_WINDOW['xa'] < J1_FACE_X <= USB_WINDOW['xb'], 'USB window does not reach the J1 face')

    jacks = []
    for local_x, local_y in JACK_LOCAL:
        jx, jy = enc_from_local(local_x, local_y)
        require(DB25_WINDOW['ya'] <= jy - JACK_RADIUS and jy + JACK_RADIUS <= DB25_WINDOW['yb'],
                f'4-40 clearance at Y={jy} does not fit the window')
        jacks.append({'x': jx, 'y': jy, 'radius': JACK_RADIUS})

    fin_near = FIN_CY[1] + FIN_HALF_Y
    require(fin_near <= envelope['ya'] - 2.0, 'right fin is closer than 2 mm to the DB25 envelope')
    pad_near = PAD_C[-1][1] + PAD_HALF_Y
    require(pad_near <= envelope['ya'] - 2.0, 'corner pad is closer than 2 mm to the DB25 envelope')
    require(RIB_X[1] <= envelope['xa'] - 2.0, 'lid rib is closer than 2 mm to the DB25 envelope')
    for sx in SCREW_X:
        require(sx + SCREW_OD / 2 < PCB_X[0] or sx - SCREW_OD / 2 > PCB_X[1],
                'a screw tower overlaps the board')

    recess = rnd(EXT_X[1] - shell_front_x)
    return {
        'drill_file': DRILL_REL,
        'drill_created': date_match.group(1) + 'Z',
        'drill_note': 'r7.1 Excellon. This repository has no r7.2 Gerber; the r7.2 PCB places J2 on the same holes.',
        'footprint_sha256': fp_hash,
        'outer_row_x': row2_x,
        'edge_distance_mm': rnd(PCB_X[1] - row2_x),
        'flange_x': flange_x,
        'shell_front_x': shell_front_x,
        'shell_top_z': rnd(PCB_Z[1] + SHELL_ABOVE_BOARD),
        'outer_wall_x': EXT_X[1],
        'mating_face_recess_mm': recess,
        'courtyard_mm': {'x': [cr_xa, cr_xb], 'y': [cr_ya, cr_yb]},
        'envelope': envelope,
        'jackscrews': jacks,
        'db25_window_mm': DB25_WINDOW,
        'usb_window_mm': USB_WINDOW,
    }


def parse_args(argv):
    parser = argparse.ArgumentParser(description='Build or check the r7.2 first-fit enclosure')
    parser.add_argument('--check', action='store_true', help='verify the drill, footprint, and envelope only')
    parser.add_argument('--update-records', action='store_true',
                        help='copy the fresh report, parameters, and hashes next to this script')
    return parser.parse_args(argv)


def normalize_step(path):
    text = open(path, encoding='utf-8').read()
    head, sep, rest = text.partition('ENDSEC;')
    require(bool(sep), 'STEP header missing in ' + path)
    head = re.sub(r'\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}', '2026-01-01T00:00:00', head)
    with open(path, 'w', encoding='utf-8', newline='\n') as stream:
        stream.write(head + sep + rest)


def write_text(path, text):
    with open(path, 'w', encoding='utf-8', newline='\n') as stream:
        stream.write(text)


def _volume(workplane):
    solids = workplane.vals()
    return sum(solid.Volume() for solid in solids) if solids else 0.0


def _probe(housing, xa, xb, ya, yb, za, zb):
    import cadquery as cq
    box = (cq.Workplane('XY')
           .box(xb - xa, yb - ya, zb - za, centered=(True, True, True))
           .translate(((xa + xb) * .5, (ya + yb) * .5, (za + zb) * .5)))
    return _volume(housing.intersect(box))


def opening_assessment(spec, housing):
    """Record the DB25 opening against NorComp 182-YYY-213RYY1 rev 18 after the first-fit wall thicken."""
    flange_width = 53.04
    flange_tol = 0.38
    shell_width = 38.40
    shell_tol = 0.25
    nose = 5.90
    nose_tol = 0.25
    height = 12.55
    general_tol = 0.15
    tail = 3.18
    tail_tol = 0.38
    overall_depth = 18.30
    depth_tol = 0.25
    center_y = rnd((spec['jackscrews'][0]['y'] + spec['jackscrews'][1]['y']) * .5)
    half_flange = rnd((flange_width + flange_tol) * .5)
    flange_y = [rnd(center_y - half_flange), rnd(center_y + half_flange)]
    window_y = DB25_WINDOW['yb'] - DB25_WINDOW['ya']
    window_z = DB25_WINDOW['zb'] - DB25_WINDOW['za']
    probe_x = (160.5, 162.8)
    span_y = DB25_WINDOW['yb'] - DB25_WINDOW['ya']
    thick = probe_x[1] - probe_x[0]
    below = _probe(housing, *probe_x, DB25_WINDOW['ya'], DB25_WINDOW['yb'], BASE_Z[0], DB25_WINDOW['za'])
    above = _probe(housing, *probe_x, DB25_WINDOW['ya'], DB25_WINDOW['yb'], DB25_WINDOW['zb'], LID_Z[1])
    below_h = round(below / (thick * span_y), 2)
    above_h = round(above / (thick * span_y), 2)
    nominal_below = round(DB25_WINDOW['za'] - BASE_Z[0], 2)
    nominal_above = round(LID_Z[1] - DB25_WINDOW['zb'], 2)
    require(abs(below_h - nominal_below) <= 0.3, f'DB25 lower ligament measured {below_h} mm')
    require(abs(above_h - nominal_above) <= 0.3, f'DB25 upper ligament measured {above_h} mm')
    drawing_front = rnd(PCB_X[1] + nose)
    return {
        'drawing': 'NorComp 182-YYY-213RYY1 rev 18, 25-pin',
        'opening_changed': True,
        'window_size_mm': {'y': round(window_y, 2), 'z': round(window_z, 2)},
        'flange_width_mm': flange_width,
        'flange_width_tolerance_mm': flange_tol,
        'front_shell_width_mm': shell_width,
        'front_shell_width_tolerance_mm': shell_tol,
        'nose_past_flange_mm': nose,
        'nose_tolerance_mm': nose_tol,
        'height_above_board_mm': height,
        'general_tolerance_mm': general_tol,
        'pin_tail_below_board_mm': tail,
        'pin_tail_tolerance_mm': tail_tol,
        'overall_depth_front_to_rear_mm': overall_depth,
        'overall_depth_tolerance_mm': depth_tol,
        'max_flange_y_mm': flange_y,
        'margin_beyond_max_flange_mm': [
            round(flange_y[0] - DB25_WINDOW['ya'], 2),
            round(DB25_WINDOW['yb'] - flange_y[1], 2),
        ],
        'drawing_nose_front_x': drawing_front,
        'kicad_graphic_front_x': spec['shell_front_x'],
        'measured_ligament_mm': {
            'probe_x_mm': list(probe_x),
            'below_window': below_h,
            'above_window': above_h,
        },
        'hardware_option': '53 is a 4-40 flush insert plus board locks. The 4.8 mm and 5.8 mm hex heads on sheet 2 belong to other hardware options.',
        'decision': 'FIRST-FIT thicken: DB25 window shrunk without mating-hood or thumbscrew drawings. Width 58.0 mm keeps >=2.2 mm beyond max flange (53.42 mm) and still clears 4-40 ear circles. Height top cut to zb=18.0 so upper ligament is ~5.0 mm (was 2.9 mm). USB window also tightened around J1 STEP body with ~1.5–3 mm documented clearance. Physical plug/hood fit still required before lock.',
    }


def build(spec, update_records):
    import cadquery as cq
    import trimesh
    from pcb_step_clearance import check_pcb_step

    os.makedirs(OUT, exist_ok=True)
    envelope = spec['envelope']

    def rounded_prism(xa, xb, ya, yb, za, zb, radius=0):
        part = (cq.Workplane('XY')
                .box(xb - xa, yb - ya, zb - za, centered=(True, True, True))
                .translate(((xa + xb) * .5, (ya + yb) * .5, (za + zb) * .5)))
        if radius:
            part = part.edges('|Z').fillet(radius)
        return part

    def box_at(xa, xb, ya, yb, za, zb):
        return rounded_prism(xa, xb, ya, yb, za, zb, 0)

    base = rounded_prism(*EXT_X, *EXT_Y, *BASE_Z, OUTER_R)
    inner = rounded_prism(EXT_X[0] + WALL, EXT_X[1] - WALL, EXT_Y[0] + WALL, EXT_Y[1] - WALL,
                          FLOOR_TOP, BASE_Z[1] + .7, INNER_R)
    base = base.cut(inner)

    for cx, cy in PAD_C:
        base = base.union(box_at(cx - PAD_HALF_X, cx + PAD_HALF_X, cy - PAD_HALF_Y, cy + PAD_HALF_Y, FLOOR_TOP, 0.0))

    for cy in FIN_CY:
        base = base.union(box_at(-3.55, -1.05, cy - FIN_HALF_Y, cy + FIN_HALF_Y, FLOOR_TOP, 3.1))
        base = base.union(box_at(*RIGHT_FIN_X, cy - FIN_HALF_Y, cy + FIN_HALF_Y, FLOOR_TOP, 3.1))
    base = base.union(box_at(104, 116, 0.90, 3.12, FLOOR_TOP, 3.1))
    base = base.union(box_at(104, 116, -121.12, -118.90, FLOOR_TOP, 3.1))

    for sx in SCREW_X:
        for sy in SCREW_Y:
            tower = (cq.Workplane('XY').center(sx, sy).circle(SCREW_OD / 2)
                     .extrude(BASE_Z[1] - FLOOR_TOP).translate((0, 0, FLOOR_TOP)))
            base = base.union(tower)

    base = base.cut(box_at(**USB_WINDOW))
    base = base.cut(box_at(**DB25_WINDOW))

    for sx in SCREW_X:
        for sy in SCREW_Y:
            drill = (cq.Workplane('XY').center(sx, sy)
                     .circle(SCREW_PILOT / 2).extrude(9.5).translate((0, 0, BASE_Z[1] - 9.5)))
            base = base.cut(drill)

    lid = rounded_prism(*EXT_X, *EXT_Y, *LID_Z, OUTER_R)
    lid = lid.cut(rounded_prism(EXT_X[0] + WALL, EXT_X[1] - WALL, EXT_Y[0] + WALL, EXT_Y[1] - WALL,
                                LID_Z[0] - .4, ROOF_UNDER, INNER_R))
    for cx, cy in PAD_C:
        lid = lid.union(box_at(cx - 2.65, cx + 2.65, cy - 4.7, cy + 4.7, 2.2, ROOF_UNDER))
    for sx in SCREW_X:
        for sy in SCREW_Y:
            tower = (cq.Workplane('XY').center(sx, sy).circle(SCREW_OD / 2)
                     .extrude(ROOF_UNDER - LID_Z[0]).translate((0, 0, LID_Z[0])))
            lid = lid.union(tower)
    lid = lid.union(box_at(*RIB_X, -59.4, -57.1, 17.6, ROOF_UNDER))
    lid = lid.union(box_at(110.0, 112.3, -105, -18.0, 17.6, ROOF_UNDER))
    lid = lid.cut(box_at(**USB_WINDOW))
    lid = lid.cut(box_at(**DB25_WINDOW))
    for sx in SCREW_X:
        for sy in SCREW_Y:
            thru = (cq.Workplane('XY').center(sx, sy).circle(SCREW_CLEAR / 2)
                    .extrude(LID_Z[1] - LID_Z[0] + .2).translate((0, 0, LID_Z[0] - .1)))
            lid = lid.cut(thru)
            counterbore = (cq.Workplane('XY').center(sx, sy).circle(6.0 / 2)
                           .extrude(1.8).translate((0, 0, LID_Z[1] - 1.8)))
            lid = lid.cut(counterbore)

    skipped = []
    for caption, size, y, depth in (('USB-DB25', 8.8, -50, 0.48),
                                    ('EEG MARKER BOX', 5.0, -63, 0.40),
                                    ('r7.2  /  FIRST FIT', 2.8, -74, 0.38)):
        try:
            cuts = cq.Workplane('XY', origin=(77.5, y, LID_Z[1] + .02)).text(
                caption, size, -depth - .04, font='DejaVu Sans',
                kind='bold' if size > 7 else 'regular', halign='center', valign='center', combine=False)
            lid = lid.cut(cuts)
        except Exception as exc:
            skipped.append(f'{caption}: {exc!r}')
    require(not skipped, 'lid text failed: ' + '; '.join(skipped))

    for x1, x2, y1, y2 in ((10, 29, -38, -37), (128, 146, -38, -37)):
        lid = lid.cut(box_at(x1, x2, y1, y2, LID_Z[1] - .4, LID_Z[1] + .05))

    print('Building final solid geometry...', flush=True)
    base = base.clean()
    lid = lid.clean()
    for label, solid in (('base', base), ('lid', lid)):
        bounds = solid.val().BoundingBox()
        print(label, 'valid', solid.val().isValid(), 'solids', len(solid.val().Solids()),
              'volume mm3', round(solid.val().Volume()),
              'bbox', tuple(round(v, 2) for v in (bounds.xmin, bounds.xmax, bounds.ymin, bounds.ymax, bounds.zmin, bounds.zmax)),
              flush=True)
        require(len(solid.val().Solids()) == 1 and solid.val().isValid(), label + ' is not a valid solid')

    keepouts = [
        ('J1', 0.08, 7.64, -49.42, -40.48, 0.60, 4.77),
        ('U2', 53.17, 74.99, -13.15, -3.94, -2.40, 12.80),
        ('J3', 12.63, 15.29, -63.78, -53.60, -1.40, 10.14),
        ('U6', 58.65, 66.57, -113.56, -103.76, -1.90, 5.46),
        ('DB25-component-envelope', envelope['xa'], envelope['xb'], envelope['ya'], envelope['yb'],
         envelope['za'], envelope['zb']),
    ]
    housing = base.union(lid)
    for name, xa, xb, ya, yb, za, zb in keepouts:
        hit = housing.intersect(box_at(xa, xb, ya, yb, za, zb))
        solids = hit.vals()
        volume = sum(item.Volume() for item in solids) if solids else 0.0
        print(f'keepout {name} intersection mm3 {volume:.2f}', flush=True)
        require(volume < 1.0, name + ' intersects the enclosure')

    print('Checking every solid in the PCB STEP...', flush=True)
    opening = opening_assessment(spec, housing)
    clearance = check_pcb_step(housing, ROOT)
    require(not clearance['collision_refs'], 'PCB solids intersect the enclosure: ' + ', '.join(clearance['collision_refs']))
    print('nearest component', clearance['minimum_component_clearance_ref'],
          clearance['minimum_component_clearance_mm'], 'mm', flush=True)
    print('unverified', ', '.join(clearance['unverified_refs']), flush=True)

    for label, solid in (('base', base), ('lid', lid)):
        print('Writing', label, 'STEP...', flush=True)
        cq.exporters.export(solid, os.path.join(OUT, f'{label}_r72.step'))
        print('Writing', label, 'STL...', flush=True)
        cq.exporters.export(solid, os.path.join(OUT, f'{label}_r72.stl'), tolerance=.09, angularTolerance=.2)

    assembly = cq.Assembly(name='USB-DB25 Marker Box prototype')
    assembly.add(base, name='BASE', color=cq.Color(0.16, 0.16, 0.18))
    assembly.add(lid, name='LID', color=cq.Color(0.10, 0.11, 0.14))
    assembly.save(os.path.join(OUT, 'assembled_enclosure_r72.step'))
    for name in ('base_r72.step', 'lid_r72.step', 'assembled_enclosure_r72.step'):
        normalize_step(os.path.join(OUT, name))

    mesh_parts = []
    for label, solid in (('base', base), ('lid', lid)):
        brep = solid.val()
        bounds = brep.BoundingBox()
        brep_bounds = [[round(bounds.xmin, 3), round(bounds.ymin, 3), round(bounds.zmin, 3)],
                       [round(bounds.xmax, 3), round(bounds.ymax, 3), round(bounds.zmax, 3)]]
        stl_path = os.path.join(OUT, f'{label}_r72.stl')
        step_path = os.path.join(OUT, f'{label}_r72.step')
        mesh = trimesh.load_mesh(stl_path, force='mesh', process=True)
        mesh_bounds = [[float(v) for v in row] for row in mesh.bounds.tolist()]
        brep_volume = float(brep.Volume())
        mesh_volume = abs(float(mesh.volume))
        relative_volume_error = abs(mesh_volume - brep_volume) / max(brep_volume, 1.0)
        bound_error = max(abs(mesh_bounds[a][b] - brep_bounds[a][b]) for a in (0, 1) for b in (0, 1, 2))
        require(brep.isValid(), label + ' B-rep is invalid')
        require(len(brep.Solids()) == 1, label + ' must export as one solid')
        require(mesh.is_watertight, label + ' STL is not watertight')
        require(mesh.is_winding_consistent, label + ' STL winding is inconsistent')
        require(relative_volume_error < 0.03, label + ' STL/B-rep volume mismatch exceeds 3%')
        require(bound_error < 0.25, label + ' STL/B-rep bounds mismatch exceeds 0.25 mm')
        mesh_parts.append({
            'part': label,
            'brep_valid': bool(brep.isValid()),
            'brep_solid_count': len(brep.Solids()),
            'brep_volume_mm3': round(brep_volume, 3),
            'brep_bounds_mm': brep_bounds,
            'stl_watertight': bool(mesh.is_watertight),
            'stl_winding_consistent': bool(mesh.is_winding_consistent),
            'stl_volume_mm3': round(mesh_volume, 3),
            'stl_relative_volume_error': round(relative_volume_error, 6),
            'stl_face_count': int(len(mesh.faces)),
            'stl_bounds_mm': [[round(v, 3) for v in row] for row in mesh_bounds],
            'step_size_bytes': os.path.getsize(step_path),
            'stl_size_bytes': os.path.getsize(stl_path),
            'step_sha256': sha256_file(step_path),
            'stl_sha256': sha256_file(stl_path),
        })

    report = {
        'status': 'PASS_WITH_UNVERIFIED_GAPS',
        'cadquery_version': cq.__version__,
        'trimesh_version': trimesh.__version__,
        'source_sha256': sha256_file(os.path.abspath(__file__)),
        'clearance_source_sha256': sha256_file(os.path.join(HERE, 'pcb_step_clearance.py')),
        'validation_scope': 'drill and footprint placement, courtyard envelope, wall opening, B-rep validity, STL watertightness, volume and bounds',
        'db25_component_envelope_mm': {
            'x': [envelope['xa'], envelope['xb']],
            'y': [envelope['ya'], envelope['yb']],
            'z': [envelope['za'], envelope['zb']],
        },
        'db25_window_mm': DB25_WINDOW,
        'db25_fit_mm': {
            'shell_front_x': spec['shell_front_x'],
            'flange_x': spec['flange_x'],
            'outer_wall_x': spec['outer_wall_x'],
            'mating_face_recess_mm': spec['mating_face_recess_mm'],
            'shell_top_z': spec['shell_top_z'],
            'jackscrews': spec['jackscrews'],
            'courtyard_mm': spec['courtyard_mm'],
        },
        'drill_file': spec['drill_file'],
        'drill_created': spec['drill_created'],
        'db25_note': 'Envelope is the KiCad courtyard plus 0.40 mm. The uploaded STEP has no DB25 body. Opening resized for FIRST-FIT wall thicken; hood/thumbscrew still undrawn.',
        'db25_opening_review': opening,
        'pcb_clearance': clearance,
        'parts': mesh_parts,
    }
    notes = [
        f"Hole positions checked in {spec['drill_file']} ({spec['drill_created']}). That file is the r7.1 drill.",
        'No r7.2 Gerber is in this repository. The r7.2 PCB still places J2 at (144.46, 58), rotation +90, and its footprint matches the r7.1 footprint.',
        'The outer signal row is X=147.30, 7.70 mm from the right edge X=155. The flange is on that edge and the socket faces out.',
        'The keep-out is the courtyard plus 0.40 mm on each side, including the inboard end. It is not a NorComp solid.',
        f"Shell front X={spec['shell_front_x']}, outer face X={spec['outer_wall_x']}, so the mating face is {spec['mating_face_recess_mm']} mm behind the outer face. The window already removes that wall.",
        'DB25 opening resized for FIRST-FIT: width 58.0 x height 23.2 mm (was 61.0 x 25.3). Upper ligament target ~5.0 mm. NorComp rev 18 still lacks mating hood and option-53 thumbscrew size; clearance is documented conservative guess.',
        'USB-C window tightened to y=-53..-37, z=-1.5..8.5 around J1 STEP body (still reaches face x=0.08). No cable-plug drawing; leave ~1.5 mm+ side clearance.',
        'Every solid in the uploaded PCB STEP was checked. Placeholder cubes and PCB references missing from that STEP stay unverified.',
        'Screws are outside the board. Do not bridge USB ground and GND_ISO with metal hardware.',
        'Do not connect this board to an EEG instrument before electrical validation.',
    ]
    params = {
        'design_type': 'first-fit prototyping - do not release without test fit',
        'board_outer_mm': [155, 118, 1.6],
        'enclosure_outer_mm': [EXT_X[1] - EXT_X[0], EXT_Y[1] - EXT_Y[0], LID_Z[1] - BASE_Z[0]],
        'coord_system': 'aligned with supplied EasyEDA 3D STEP',
        'material_recommendation': 'PETG / PA12; 0.2 mm layers; 3-4 perimeters, 25-35% infill',
        'wall_thickness_mm': WALL,
        'floor_roof_mm': [rnd(FLOOR_TOP - BASE_Z[0]), rnd(LID_Z[1] - ROOF_UNDER)],
        'board_lower_z_mm': 0,
        'board_upper_z_mm': 1.6,
        'lowest_observed_3d_part_z_mm': -2.4,
        'highest_observed_3d_part_z_mm': 12.8,
        'lid_roof_lower_z_mm': ROOF_UNDER,
        'pcba_mount': 'four support ledges, four compliant top retainers, lateral fins; no PCB holes needed',
        'retainer_gap_above_board_mm': 0.6,
        'foam_pad_recommendation_mm': 0.8,
        'fasteners': '4 x M3 self-tapping screws for PETG, nominal M3x25; verify engagement and screw head',
        'pilot_diameter_mm': SCREW_PILOT,
        'clearance_diameter_mm': SCREW_CLEAR,
        'db25_component_envelope_mm': report['db25_component_envelope_mm'],
        'db25_fit_mm': report['db25_fit_mm'],
        'db25_opening_review': {
            'opening_changed': opening['opening_changed'],
            'window_size_mm': opening['window_size_mm'],
            'measured_ligament_mm': opening['measured_ligament_mm'],
            'margin_beyond_max_flange_mm': opening['margin_beyond_max_flange_mm'],
            'decision': opening['decision'],
        },
        'pcb_clearance_summary': {
            'modeled_component_count': clearance['modeled_component_count'],
            'collision_refs': clearance['collision_refs'],
            'minimum_component_clearance_mm': clearance['minimum_component_clearance_mm'],
            'minimum_component_clearance_ref': clearance['minimum_component_clearance_ref'],
            'unverified_refs': clearance['unverified_refs'],
        },
        'drill_file': spec['drill_file'],
        'drill_created': spec['drill_created'],
        'notes': notes,
        'usb_window_mm': USB_WINDOW,
        'db25_window_mm': DB25_WINDOW,
        'screw_xy_mm': {'x': list(SCREW_X), 'y': list(SCREW_Y)},
    }
    reference_lines = [
        '# Generated by build_marker_enclosure.py --update-records',
        '# CadQuery 2.6.1. STEP FILE_NAME timestamps are normalized before hashing.',
        '# STL hashes identify the printable solids. Do not edit this file by hand.',
        f"{report['source_sha256']}  build_marker_enclosure.py",
        f"{report['clearance_source_sha256']}  pcb_step_clearance.py",
    ]
    for part in mesh_parts:
        reference_lines.append(f"{part['step_sha256']}  {part['part']}_r72.step")
        reference_lines.append(f"{part['stl_sha256']}  {part['part']}_r72.stl")
    reference_lines.append('')
    records = {
        'mesh_verification.json': json.dumps(report, indent=2, ensure_ascii=False) + '\n',
        'params.json': json.dumps(params, indent=2, ensure_ascii=False) + '\n',
        'SHA256_REFERENCE.txt': '\n'.join(reference_lines),
    }
    targets = [OUT]
    if update_records:
        targets.append(HERE)
    for folder in targets:
        os.makedirs(folder, exist_ok=True)
        for name, text in records.items():
            write_text(os.path.join(folder, name), text)
    print('STL mesh verification passed', flush=True)
    print('mating face recess mm', spec['mating_face_recess_mm'], flush=True)


def main(argv):
    args = parse_args(argv)
    spec = connector_spec()
    if args.check:
        envelope = spec['envelope']
        print('DB25 faces out')
        print(f"drill {spec['drill_created']} outer row X={spec['outer_row_x']} edge distance {spec['edge_distance_mm']} mm")
        print(f"envelope x {envelope['xa']}..{envelope['xb']} y {envelope['ya']}..{envelope['yb']} z {envelope['za']}..{envelope['zb']}")
        print(f"mating face X={spec['shell_front_x']} recess {spec['mating_face_recess_mm']} mm behind outer X={spec['outer_wall_x']}")
        print('connector spec ok')
        return
    build(spec, args.update_records)


if __name__ == '__main__':
    try:
        main(sys.argv[1:])
    except AssertionError as exc:
        print('CHECK FAILED:', exc, file=sys.stderr)
        sys.exit(1)

