"""C prototype: two PA12 printed parts, integral snaps and PCB retention.

No screws, inserts, foam, adhesive, or separate clip parts are required.
CAD uses the established PCB frame; STL files are oriented on Z=0 for printing.
Run normally for geometry + source-PCB checks; --geometry-only is for CAD iteration.
"""
from pathlib import Path
import argparse
import hashlib
import importlib.util
import json
import math
import sys

import cadquery as cq
import trimesh

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
OUT = HERE / 'output'
spec_loader = importlib.util.spec_from_file_location(
    'rail_reference', HERE.parent / 'rail-fit/build_marker_enclosure.py')
B = importlib.util.module_from_spec(spec_loader)
spec_loader.loader.exec_module(B)
sys.path.insert(0, str(HERE.parent / 'first-fit'))
from pcb_step_clearance import check_pcb_step

EXT_X = (-8.0, 163.0)
EXT_Y = (-131.0, 13.0)
WALL = 2.8
CAVITY_Y = (-128.4, 10.4)  # 2.6 mm long walls allow 0.6 mm behind each spring.
FLOOR = -7.2
ROOF = 20.2
SPLIT = 4.5
LIP_GAP = 0.4
LIP_T = 1.2
CLIP_X = (38.0, 117.0)
CLIP_W = 8.0
CLIP_T = 1.2
CLIP_ROOT_R = 0.8
CLIP_ROOT_Z = 19.4
CLIP_BOTTOM = -3.5
CLIP_PAD_Y = 12.0
CLIP_HOOK_Y = 11.2
WINDOW_Z = (-4.2, 1.4)
PCB_STOP_Z = 2.05
PIN_D = 1.4
PIN_H = 1.6


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def volume(wp):
    return sum(s.Volume() for s in wp.vals())


def box(xa, xb, ya, yb, za, zb, radius=0):
    part = (cq.Workplane('XY').box(xb-xa, yb-ya, zb-za)
            .translate(((xa+xb)/2, (ya+yb)/2, (za+zb)/2)))
    return part.edges('|Z').fillet(radius) if radius else part


def mirrored(part, side):
    return part if side == 1 else part.mirror('XZ').translate((0, -118, 0))


def cylinder(x, y, radius, bottom, top):
    return cq.Workplane('XY', origin=(x, y, bottom)).circle(radius).extrude(top-bottom)


def ring(bounds, thickness, z0, z1, radius):
    xa, xb, ya, yb = bounds
    return box(xa, xb, ya, yb, z0, z1, radius).cut(
        box(xa+thickness, xb-thickness, ya+thickness, yb-thickness,
            z0-.1, z1+.1, max(radius-thickness, .2)))


def clip_at(cx, side):
    # YZ beam: root has two 0.8 mm tangent arcs; the beam joins the roof/wall.
    delta = CLIP_ROOT_R / math.sqrt(2)
    beam = (cq.Workplane('YZ', origin=(cx-CLIP_W/2, 0, 0))
            .moveTo(8.6, CLIP_BOTTOM).lineTo(9.8, CLIP_BOTTOM)
            .lineTo(9.8, CLIP_ROOT_Z)
            .threePointArc((10.6-delta, CLIP_ROOT_Z+delta), (10.6, ROOF))
            .lineTo(10.6, 21.0).lineTo(7.8, 21.0).lineTo(7.8, ROOF)
            .threePointArc((7.8+delta, CLIP_ROOT_Z+delta), (8.6, CLIP_ROOT_Z))
            .close().extrude(CLIP_W))
    # Leading ramp deflects the beam inward during closure. The low pad is
    # accessible through the base window; the upper shoulder retains the lid.
    tooth = (cq.Workplane('YZ', origin=(cx-CLIP_W/2, 0, 0))
             .polyline([(9.7,-3.5), (CLIP_PAD_Y,-1.4), (CLIP_PAD_Y,-.7),
                        (CLIP_HOOK_Y,-.7), (CLIP_HOOK_Y,1.0), (9.7,1.0)])
             .close().extrude(CLIP_W))
    return mirrored(beam.union(tooth), side)


def board_rails():
    result = None
    for ya, yb in ((0.0,5.0),(-123.0,-118.0)):
        rail = box(0,155,ya,yb,0,1.6)
        for hx,hy in B.RAIL_HOLES:
            rail = rail.cut(cylinder(hx,hy,B.RAIL_HOLE_D/2,-.1,1.7))
        result = rail if result is None else result.union(rail)
    return result


def build_geometry():
    base = box(*EXT_X,*EXT_Y,-10,SPLIT,5).cut(
        box(-5.2,160.2,*CAVITY_Y,FLOOR,SPLIT+.5,2.2))
    # Main-board bearing pads remain; all bearing faces are at PCB lower Z=0.
    for cx,cy in B.PAD_C:
        base = base.union(box(cx-4,cx+4,cy-5,cy+5,FLOOR,0))
    for cy in B.FIN_CY:
        base = base.union(box(-3.15,-.35,cy-4.5,cy+4.5,FLOOR,3.1))
        base = base.union(box(155.35,158.2,cy-4.5,cy+4.5,FLOOR,3.1))
    for cx in (65.0,95.0):
        for side in (1,-1):
            base = base.union(mirrored(box(cx-3,cx+3,5.35,8.2,FLOOR,2.0),side))
    for hx,hy in B.RAIL_HOLES:
        base = base.union(cylinder(hx,hy,2.3,FLOOR,0))
    # Two diagonal locating pins. The second is relieved along the line
    # between holes so the long board is not constrained by four tight pins.
    hx,hy = B.RAIL_HOLES[0]
    pin = cylinder(hx,hy,PIN_D/2,0,1.2).union(cq.Workplane(obj=
        cq.Solid.makeCone(PIN_D/2,.45,.4,cq.Vector(hx,hy,1.2))))
    base = base.union(pin)
    hx,hy = B.RAIL_HOLES[3]
    relief_pin = cq.Workplane('XY').ellipse(.5,.7).extrude(1.2)
    tip = (cq.Workplane('XY',origin=(0,0,1.2)).ellipse(.5,.7)
           .workplane(offset=.4).ellipse(.3,.42).loft())
    angle = math.degrees(math.atan2(-123,145))
    relief_pin = relief_pin.union(tip).rotate((0,0,0),(0,0,1),angle).translate((hx,hy,0))
    base = base.union(relief_pin)

    lid = box(*EXT_X,*EXT_Y,SPLIT,23,5).cut(
        box(-5.2,160.2,*CAVITY_Y,SPLIT-.5,ROOF,2.2))
    # Solid rail stops replace the B-version corner pads/EVA arrangement.
    for hx,hy in B.RAIL_HOLES:
        lid = lid.union(cylinder(hx,hy,2.3,PCB_STOP_Z,ROOF))
    lid = lid.union(box(4,138,-59.4,-57.1,17.6,ROOF))
    lid = lid.union(box(110,112.3,-105,-18,17.6,ROOF))

    # A recessed rim transfers lateral loads and aligns the halves.
    lip = ring((-4.8,159.8,-128.0,10.0),LIP_T,2.5,5.5,1.8)
    root = ring((-5.3,160.3,-128.5,10.5),1.7,SPLIT,5.5,2.3)
    lid = lid.union(lip).union(root)
    for cx in CLIP_X:
        for side in (1,-1):
            relief = mirrored(box(cx-4.6,cx+4.6,7.0,CAVITY_Y[1],2.4,19.3),side)
            lid = lid.cut(relief)
            aperture = box(cx-6,cx+6,10.0,13.4,*WINDOW_Z).edges('|Y').fillet(.65)
            base = base.cut(mirrored(aperture,side))
            lid = lid.union(clip_at(cx,side))

    for window in (B.USB_WINDOW,B.DB25_WINDOW):
        base = base.cut(box(**window))
        lid = lid.cut(box(**window))
    for text,size,y in (('USB-DB25',8.8,-50),('EEG MARKER BOX',5,-63),
                        ('r7.2 / SNAP C / PA12',2.8,-74)):
        engraving = cq.Workplane('XY',origin=(77.5,y,23.02)).text(
            text,size,-.42,font='DejaVu Sans',kind='bold' if size>7 else 'regular',
            halign='center',valign='center',combine=False)
        lid = lid.cut(engraving)
    for number,cx in enumerate(CLIP_X,1):
        for y in (7,-125):
            label = cq.Workplane('XY',origin=(cx,y,23.02)).text(
                str(number),3,-.32,font='DejaVu Sans',halign='center',valign='center',combine=False)
            lid = lid.cut(label)
    return base.clean(),lid.clean()


def geometry_checks(base,lid,spec):
    for name,part in (('base',base),('lid',lid)):
        require(part.val().isValid() and len(part.val().Solids())==1,
                name+' must be one valid solid')
    overlap = volume(base.intersect(lid))
    require(overlap<.0001,'closed base/lid overlap: '+str(overlap))
    housing = base.union(lid)
    rail_overlap = volume(housing.intersect(board_rails()))
    require(rail_overlap<.0001,'housing overlaps the CAM rails: '+str(rail_overlap))
    envelope = spec['envelope']
    connector_overlap = volume(housing.intersect(box(**envelope)))
    require(connector_overlap<.0001,'housing overlaps the DB25 envelope')
    # The wide release pad passes the base wall before entering its window.
    # Include 0.2 mm clearance, and account for the free beam overhang below
    # the contact point. This is a beam/design screen, not FEA or fatigue data.
    contact_z = -.7
    displacement = CLIP_PAD_Y-CAVITY_Y[1]+.2
    length = CLIP_ROOT_Z-contact_z
    free_tip_displacement = displacement+(contact_z-CLIP_BOTTOM)*1.5*displacement/length
    strain = 1.5*CLIP_T*displacement/(length*length)
    require(strain<.015,'nominal cantilever strain exceeds design screening target')
    require(8.6-free_tip_displacement-5>=1.0,'deflected clip approaches the rail edge')
    # Rigid stops cannot make a positioning pin leave its hole under the
    # nominal 0.4 mm latch free play, even at the 1.44 mm board-thickness case.
    max_lift = PCB_STOP_Z-1.44+.4
    require(PIN_H-max_lift>.1,'not enough residual locating-pin engagement')
    return {
        'base_lid_intersection_mm3':overlap,
        'cam_rail_intersection_mm3':rail_overlap,
        'db25_envelope_intersection_mm3':connector_overlap,
        'minimum_clip_to_rail_gap_at_screened_tip_deflection_mm':8.6-free_tip_displacement-5,
        'snap_beam_back_clearance_mm':CAVITY_Y[1]-9.8,
        'snap_nominal_lateral_engagement_mm':CLIP_HOOK_Y-CAVITY_Y[1],
        'snap_nominal_vertical_free_play_mm':WINDOW_Z[1]-1.0,
        'snap_screening':{'model':'small-deflection uniform cantilever, epsilon=1.5*t*delta/L^2',
                          'length_mm':length,'thickness_mm':CLIP_T,'width_mm':CLIP_W,
                          'pad_deflection_screen_mm':displacement,
                          'overhanging_tip_deflection_mm':free_tip_displacement,'strain_fraction':strain,
                          'screening_target_fraction':.015,
                          'qualification':'nominal design screen only; no FEA, force or cycle testing'},
        'pcb_vertical_gap_nominal_mm':PCB_STOP_Z-1.6,
        'pcb_vertical_gap_at_1p76mm_board_mm':PCB_STOP_Z-1.76,
        'pin_engagement_at_1p44mm_board_and_max_lid_play_mm':PIN_H-max_lift,
    }


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--geometry-only',action='store_true')
    args=parser.parse_args()
    OUT.mkdir(exist_ok=True)
    spec=B.connector_spec()
    print('Building C snap-fit geometry...',flush=True)
    base,lid=build_geometry()
    checks=geometry_checks(base,lid,spec)
    print('Geometry checks passed',json.dumps(checks),flush=True)
    housing=base.union(lid)
    if args.geometry_only:
        clearance={'status':'NOT_RUN'}
    else:
        print('Checking all modeled PCB components...',flush=True)
        clearance=check_pcb_step(housing,str(ROOT))
        require(not clearance['collision_refs'],'component collisions: '+str(clearance['collision_refs']))
        print('PCB check passed; unverified:',clearance['unverified_refs'],flush=True)
    parts=[]
    print_shapes={'base':base.translate((8,131,10)),
                  'lid':lid.rotate((0,0,0),(1,0,0),180).translate((8,13,23))}
    for name,part in (('base',base),('lid',lid)):
        step=OUT/(name+'_r72_snap.step')
        stl=OUT/(name+'_r72_snap.stl')
        cq.exporters.export(part,str(step))
        B.normalize_step(str(step))
        printable=print_shapes[name]
        cq.exporters.export(printable,str(stl),tolerance=.04,angularTolerance=.12)
        mesh=trimesh.load_mesh(str(stl),process=True)
        require(mesh.is_watertight and mesh.is_winding_consistent,name+' STL topology failed')
        error=abs(mesh.volume-part.val().Volume())/part.val().Volume()
        require(error<.005,name+' mesh/solid volume mismatch')
        require(abs(mesh.bounds[0,2])<.001,name+' printable STL is not on Z=0')
        parts.append({'part':name,'brep_valid':True,'brep_solid_count':1,
                      'brep_volume_mm3':part.val().Volume(),'stl_watertight':True,
                      'stl_winding_consistent':True,'stl_relative_volume_error':float(error),
                      'stl_bounds_mm':mesh.bounds.tolist(),'step_sha256':sha(step),'stl_sha256':sha(stl)})
    assembly=cq.Assembly(name='C snap-fit prototype - two PA12 printed parts')
    assembly.add(base,name='BASE',color=cq.Color(.3,.45,.6))
    assembly.add(lid,name='LID',color=cq.Color(.85,.6,.25))
    assembly.save(str(OUT/'assembled_r72_snap.step'))
    B.normalize_step(str(OUT/'assembled_r72_snap.step'))
    # Optional two-piece fit coupon: crop the same actual snap, wall, rim and
    # release window. These are test pieces, not additional assembly parts.
    coupon_folder=OUT/'optional_coupon'
    coupon_folder.mkdir(exist_ok=True)
    coupon_parts=[]
    crop=box(CLIP_X[0]-12,CLIP_X[0]+12,4,13,-10,23.1)
    for name,part in (('base',base),('lid',lid)):
        coupon=part.intersect(crop).clean()
        require(coupon.val().isValid() and len(coupon.val().Solids())==1,'invalid '+name+' coupon')
        if name=='lid':coupon=coupon.rotate((0,0,0),(1,0,0),180)
        bounds=coupon.val().BoundingBox()
        coupon=coupon.translate((-bounds.xmin,-bounds.ymin,-bounds.zmin))
        path=coupon_folder/('clip_'+name+'.stl')
        cq.exporters.export(coupon,str(path),tolerance=.025,angularTolerance=.1)
        mesh=trimesh.load_mesh(str(path),process=True)
        require(mesh.is_watertight and mesh.is_winding_consistent,'invalid coupon STL')
        coupon_parts.append({'part':name,'stl_bounds_mm':mesh.bounds.tolist(),'stl_sha256':sha(path)})
    report={'status':'GEOMETRY_ONLY_NOT_RELEASED' if args.geometry_only else 'PASS_WITH_UNVERIFIED_GAPS',
            'source_sha256':sha(__file__),'cadquery_version':cq.__version__,
            'reference_source_sha256':sha(HERE.parent/'rail-fit/build_marker_enclosure.py'),
            'pcb_clearance_source_sha256':sha(HERE.parent/'first-fit/pcb_step_clearance.py'),
            'geometry_checks':checks,'pcb_clearance':clearance,'parts':parts,
            'optional_coupon_parts':coupon_parts}
    params={'design_type':'C removable snap-fit prototype; all retention integral to two printed parts',
            'outer_mm':[171,144,33],'end_wall_mm':WALL,'long_side_wall_mm':2.6,'floor_roof_mm':[2.8,2.8],
            'material_process':'unfilled PA12 nylon, MJF or SLS; two parts printed separately',
            'extra_assembly_materials_required':[], 'printed_part_count':2,
            'rail_holes_xy_mm':[list(h) for h in B.RAIL_HOLES],
            'rail_hole_diameter_mm':B.RAIL_HOLE_D,'rail_hole_reference':B.RAIL_HOLE_REFERENCE,
            'pcb_mount':'four solid rail pedestals; one round + one relieved diagonal locating pin; four lid stops',
            'locator_pin_diameter_mm':PIN_D,'locator_pin_height_mm':PIN_H,
            'relieved_locator_minor_major_mm':[1.0,1.4],
            'pcb_stop_lower_z_mm':PCB_STOP_Z,'pcb_nominal_vertical_gap_mm':PCB_STOP_Z-1.6,
            'snap_count':4,'snap_x_mm':list(CLIP_X),'snap_beam_thickness_width_mm':[CLIP_T,CLIP_W],
            'snap_root_radius_mm':CLIP_ROOT_R,'alignment_rim_clearance_per_side_mm':LIP_GAP,
            'snap_beam_back_clearance_mm':CAVITY_Y[1]-9.8,
            'usb_window_mm':B.USB_WINDOW,'db25_window_mm':B.DB25_WINDOW,
            'stl_pose':'print ready, separate parts on Z=0; base translated, lid inverted; STEP uses PCB assembly coordinates',
            'limits':['First physical snap/plug/PCB fit still required; no holding-force or fatigue validation.',
                      'PCB can be lifted out when the lid is removed; the assembled lid captures the PCB.',
                      'FDM PLA/PETG and standard SLA resins are not qualified for this PA12 geometry.']}
    for name,data in (('mesh_verification.json',report),('params.json',params)):
        (OUT/name).write_text(json.dumps(data,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    hashes=[f'# Generator SHA256 {sha(__file__)}  build_snap_enclosure.py (in source folder)']
    for p in sorted(OUT.iterdir()):
        if p.suffix in ('.step','.stl','.json'):hashes.append(f'{sha(p)}  {p.name}')
    for p in sorted(coupon_folder.glob('*.stl')):
        hashes.append(f'{sha(p)}  optional_coupon/{p.name}')
    (OUT/'SHA256SUMS.txt').write_text('\n'.join(hashes)+'\n',encoding='utf-8')
    print('C output:',OUT,flush=True)


if __name__=='__main__':
    main()
