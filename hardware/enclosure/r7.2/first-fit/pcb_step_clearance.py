"""Compare the enclosure with every solid in the uploaded EasyEDA PCB STEP.

A missing body or a placeholder cube is reported as unverified. It is not
counted as clearance.
"""
import hashlib
import os
import re
import tempfile
import zipfile

import cadquery as cq
from cadquery.occ_impl.shapes import Shape
from OCP.BRepBndLib import BRepBndLib
from OCP.BRepExtrema import BRepExtrema_DistShapeShape
from OCP.Bnd import Bnd_Box
from OCP.STEPCAFControl import STEPCAFControl_Reader
from OCP.TCollection import TCollection_ExtendedString
from OCP.TDF import TDF_Label, TDF_LabelSequence
from OCP.TDataStd import TDataStd_Name
from OCP.TDocStd import TDocStd_Document
from OCP.XCAFDoc import XCAFDoc_DocumentTool

STEP_ZIP = 'hardware/enclosure/r7.2/first-fit/reference/colorsilk_preview_1792580399245.zip'
STEP_MEMBER = 'colorsilk_preview_1792580399245.step'
STEP_SHA256 = 'b34f182ef6412f69bfce7aed25acf6e93a5afd71c5b33b9ebd9f6da959bf853e'
PCB_REL = 'hardware/r7.2/kicad/usb_db25_marker.kicad_pcb'
ARTWORK = {
    'TopCopper', 'BottomCopper', 'TopSilkscreen', 'BottomSilkscreen',
    'TopFpcStiffener', 'BottomFpcStiffener',
}
COLLISION_MM3 = 0.05


def _name(label):
    attr = TDataStd_Name()
    if label.FindAttribute(TDataStd_Name.GetID_s(), attr):
        return attr.Get().ToExtString()
    return ''


def _bbox(shape):
    box = Bnd_Box()
    try:
        BRepBndLib.Add_s(shape, box)
    except Exception:
        return None
    if box.IsVoid():
        return None
    return tuple(float(v) for v in box.Get())


def _round_box(box):
    return [round(v, 2) for v in box]


def _volume(workplane):
    solids = workplane.vals()
    return sum(solid.Volume() for solid in solids) if solids else 0.0


def _kicad_refs(path):
    text = open(path, encoding='utf-8').read()
    return sorted(set(re.findall(r'\(property "Reference" "([^"]+)"', text)))


def _load_step(zip_path):
    digest = hashlib.sha256()
    with zipfile.ZipFile(zip_path) as archive:
        data = archive.read(STEP_MEMBER)
    digest.update(data)
    if digest.hexdigest() != STEP_SHA256:
        raise AssertionError('PCB STEP bytes do not match the recorded SHA-256')
    temporary = tempfile.NamedTemporaryFile(suffix='.step', delete=False)
    temporary.write(data)
    temporary.close()
    try:
        document = TDocStd_Document(TCollection_ExtendedString('pcb'))
        reader = STEPCAFControl_Reader()
        reader.SetNameMode(True)
        reader.ReadFile(temporary.name)
        reader.Transfer(document)
        return document
    finally:
        os.unlink(temporary.name)


def check_pcb_step(housing, root):
    """Return a clearance report for one housing solid against the PCB STEP."""
    document = _load_step(os.path.join(root, STEP_ZIP))
    shape_tool = XCAFDoc_DocumentTool.ShapeTool_s(document.Main())
    free = TDF_LabelSequence()
    shape_tool.GetFreeShapes(free)
    components = TDF_LabelSequence()
    shape_tool.GetComponents_s(free.Value(1), components)
    housing_shape = housing.val().wrapped

    modeled = []
    unverified = []
    artwork = []
    seen = set()
    for index in range(1, components.Length() + 1):
        component = components.Value(index)
        referred = TDF_Label()
        shape_tool.GetReferredShape_s(component, referred)
        full_name = _name(referred)
        ref = full_name.split('~')[0]
        seen.add(ref)
        shape = shape_tool.GetShape_s(component)
        box = _bbox(shape)
        if ref in ARTWORK:
            artwork.append({'name': ref, 'status': 'no_solid' if box is None else 'present'})
            continue
        if box is None:
            unverified.append({'ref': ref, 'status': 'unverified', 'reason': 'no solid in the STEP'})
            continue
        size = (box[3] - box[0], box[4] - box[1], box[5] - box[2])
        if all(0.6 <= item <= 0.9 for item in size):
            unverified.append({
                'ref': ref,
                'status': 'unverified',
                'reason': 'placeholder cube, component body is not in the STEP',
                'bbox_mm': _round_box(box),
            })
            continue
        wrapped = Shape.cast(shape)
        overlap = _volume(housing.intersect(cq.Workplane(obj=wrapped)))
        distance = None
        if overlap < COLLISION_MM3:
            measure = BRepExtrema_DistShapeShape(housing_shape, shape)
            measure.Perform()
            if not measure.IsDone():
                unverified.append({'ref': ref, 'status': 'unverified', 'reason': 'distance calculation failed'})
                continue
            distance = round(float(measure.Value()), 2)
        record = {
            'ref': ref,
            'bbox_mm': _round_box(box),
            'intersection_mm3': round(overlap, 3),
        }
        if overlap >= COLLISION_MM3:
            record['status'] = 'collision'
        elif ref == 'Board' and distance is not None and distance <= 0.05:
            record['status'] = 'design_contact'
            record['clearance_mm'] = distance
            record['note'] = 'The board bottom rests on the four support pads. Overlap volume is zero.'
        elif distance is not None and distance <= 0.05:
            record['status'] = 'touch'
            record['clearance_mm'] = distance
        else:
            record['status'] = 'clear'
            record['clearance_mm'] = distance
        modeled.append(record)

    kicad_refs = _kicad_refs(os.path.join(root, PCB_REL))
    for ref in kicad_refs:
        if ref not in seen:
            unverified.append({
                'ref': ref,
                'status': 'unverified',
                'reason': 'reference is on the r7.2 PCB but has no body in the uploaded STEP',
            })
    modeled.sort(key=lambda item: item['ref'])
    unverified.sort(key=lambda item: item['ref'])
    artwork.sort(key=lambda item: item['name'])
    clearances = [item['clearance_mm'] for item in modeled if item['status'] == 'clear']
    nearest = min((item for item in modeled if item['status'] == 'clear'), key=lambda item: item['clearance_mm'])
    collisions = [item['ref'] for item in modeled if item['status'] == 'collision']
    return {
        'step_file': STEP_ZIP,
        'step_sha256': STEP_SHA256,
        'collision_mm3_threshold': COLLISION_MM3,
        'modeled_component_count': sum(1 for item in modeled if item['ref'] != 'Board'),
        'collision_refs': collisions,
        'minimum_component_clearance_mm': None if not clearances else min(clearances),
        'minimum_component_clearance_ref': None if not clearances else nearest['ref'],
        'unverified_refs': [item['ref'] for item in unverified],
        'modeled': modeled,
        'unverified': unverified,
        'artwork_without_component_body': artwork,
    }

