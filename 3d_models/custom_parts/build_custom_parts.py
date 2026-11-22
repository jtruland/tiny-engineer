#!/usr/bin/env python3
"""Build the custom hard hat and "JT" desk emblem for every servo preset.

The stock parts in parts/{servo_id}/stl/ are exported by the TinyEngineer Tools
Fusion add-in from the servos.json presets, so their size changes per servo. This
script measures each preset's Hat.stl and AiEmblem.stl, passes the measurements to
hardhat.scad / emblem_jt.scad with -D, and writes:

    parts/{servo_id}/stl/HardHat.stl
    parts/{servo_id}/stl/AiEmblemJT.stl

then checks each result:
    hard hat  - one solid; overlaps the stock Hat by contact only (< 0.1 mm^3)
    emblem    - one solid; letters reach the stock letter height; J and T each
                join the gear ring on their own (CGAL piece count)

Re-run after any Parts Exporter run. Standard library only; needs OpenSCAD.

    python build_custom_parts.py                 # every preset in servos.json
    python build_custom_parts.py sg90 es9051     # just these
"""

import argparse
import json
import math
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from collections import defaultdict
from pathlib import Path

HERE = Path(__file__).resolve().parent
MODELS = HERE.parent
PARTS = MODELS / "parts"
SERVOS_JSON = MODELS / "fusion" / "TinyEngineerTools" / "servos.json"

SG90_HAT_WIDTH = 54.1   # hard hat look was designed on this Hat width
RING_R_PAD = 0.013      # emblem cut radius = fitted inner radius + this (keeps clear of the ring)
POCKET_END_PAD = 0.4    # pocket clears the ridge's filleted base ends by this


# ---------------------------------------------------------------- mesh helpers

def load_tris(path):
    data = Path(path).read_bytes()
    if data[:5] == b"solid" and b"facet" in data[:512]:
        v = [tuple(map(float, m)) for m in re.findall(rb"vertex\s+(\S+)\s+(\S+)\s+(\S+)", data)]
        return [(v[i], v[i + 1], v[i + 2]) for i in range(0, len(v), 3)]
    n = struct.unpack("<I", data[80:84])[0]
    tris = []
    for i in range(n):
        f = struct.unpack("<12f", data[84 + i * 50:84 + i * 50 + 48])
        tris.append((f[3:6], f[6:9], f[9:12]))
    return tris


def flat_faces(tris, tol=1e-3):
    """{z: [tri, ...]} for horizontal triangles."""
    out = defaultdict(list)
    for t in tris:
        if abs(t[0][2] - t[1][2]) < tol and abs(t[0][2] - t[2][2]) < tol:
            out[round(t[0][2], 2)].append(t)
    return out


def tri_area_xy(t):
    (ax, ay, _), (bx, by, _), (cx, cy, _) = t
    return abs((bx - ax) * (cy - ay) - (cx - ax) * (by - ay)) / 2


def top_z(tris, x, y):
    """Highest surface z above (x, y), or None."""
    best = None
    for a, b, c in tris:
        d = (b[1] - c[1]) * (a[0] - c[0]) + (c[0] - b[0]) * (a[1] - c[1])
        if abs(d) < 1e-12:
            continue
        l1 = ((b[1] - c[1]) * (x - c[0]) + (c[0] - b[0]) * (y - c[1])) / d
        l2 = ((c[1] - a[1]) * (x - c[0]) + (a[0] - c[0]) * (y - c[1])) / d
        l3 = 1 - l1 - l2
        if min(l1, l2, l3) >= -1e-9:
            z = l1 * a[2] + l2 * b[2] + l3 * c[2]
            best = z if best is None or z > best else best
    return best


def clusters(points, gap):
    groups = []
    for p in points:
        for g in groups:
            if any(math.dist(p, q) < gap for q in g):
                g.append(p)
                break
        else:
            groups.append([p])
    return groups


def circle_fit(pts):
    """Algebraic least-squares circle through 2D points -> (cx, cy, r)."""
    sxx = sxy = syy = sx = sy = n = 0.0
    sxz = syz = sz = 0.0
    for x, y in pts:
        z = x * x + y * y
        sxx += x * x; sxy += x * y; syy += y * y; sx += x; sy += y; n += 1
        sxz += x * z; syz += y * z; sz += z
    m = [[sxx, sxy, sx], [sxy, syy, sy], [sx, sy, n]]
    v = [-sxz, -syz, -sz]
    for c in range(3):
        p = max(range(c, 3), key=lambda r: abs(m[r][c]))
        m[c], m[p] = m[p], m[c]; v[c], v[p] = v[p], v[c]
        for r in range(3):
            if r != c:
                f = m[r][c] / m[c][c]
                m[r] = [a - f * b for a, b in zip(m[r], m[c])]
                v[r] -= f * v[c]
    d, e, f = (v[i] / m[i][i] for i in range(3))
    cx, cy = -d / 2, -e / 2
    return cx, cy, math.sqrt(cx * cx + cy * cy - f)


def mesh_volume(tris):
    vol = 0.0
    for a, b, c in tris:
        vol += (a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0])
                + a[2] * (b[0] * c[1] - b[1] * c[0])) / 6
    return abs(vol)


def floor_centres(flat, z_lo, z_hi):
    """Centres of the counterbore floors: flat patches with z_lo < z < z_hi."""
    pts = [((t[0][0] + t[1][0] + t[2][0]) / 3, (t[0][1] + t[1][1] + t[2][1]) / 3)
           for z, ts in flat.items() if z_lo < z < z_hi for t in ts]
    out = []
    for g in clusters(pts, 5.0):
        xs = [p[0] for p in g]; ys = [p[1] for p in g]
        out.append(((min(xs) + max(xs)) / 2, (min(ys) + max(ys)) / 2))
    return out


# ---------------------------------------------------------------- measurements

def measure_hat(path):
    tris = load_tris(path)
    pts = [p for t in tris for p in t]
    xmin = min(p[0] for p in pts); xmax = max(p[0] for p in pts)
    ymin = min(p[1] for p in pts); ymax = max(p[1] for p in pts)
    zmax = max(p[2] for p in pts)
    flat = flat_faces(tris)
    # Hat top: the largest-area flat level below the ridge top
    hat_top = max((z for z in flat if 0.5 < z < zmax - 0.5),
                  key=lambda z: sum(tri_area_xy(t) for t in flat[z]))
    ridge = [p for t in flat[round(zmax, 2)] for p in t]
    rx0 = min(p[0] for p in ridge); rx1 = max(p[0] for p in ridge)
    ry = max(abs(p[1]) for p in ridge)
    # filleted base ends, scanned along a solid line of the ridge (no vent slots there)
    y_scan = 0.85 * ry
    def base_end(start, step):
        x = start
        while abs(x - start) < 8:
            z = top_z(tris, x, y_scan)
            if z is None or z <= hat_top + 0.05:
                return x
            x += step
        raise RuntimeError("ridge base end not found")
    bx0 = base_end(rx0, -0.05); bx1 = base_end(rx1, +0.05)
    screws = floor_centres(flat, 0.2, hat_top - 0.5)
    if len(screws) != 2:
        raise RuntimeError(f"expected 2 counterbores in {path}, found {len(screws)}")
    return {
        "hat_top": hat_top, "ridge_h": round(zmax - hat_top, 3),
        "ridge_x0": rx0, "ridge_x1": rx1, "ridge_y": ry,
        "pocket_x0": round(bx0 - POCKET_END_PAD, 3), "pocket_x1": round(bx1 + POCKET_END_PAD, 3),
        "screws": [[round(x, 3), round(y, 3)] for x, y in sorted(screws)],
        "center_x": round((xmin + xmax) / 2, 3),
        "shape_k": round((ymax - ymin) / SG90_HAT_WIDTH, 4),
    }


def measure_emblem(path):
    tris = load_tris(path)
    pts = [p for t in tris for p in t]
    flat = flat_faces(tris)
    letter_h = max(p[2] for p in pts)
    area = {z: sum(tri_area_xy(t) for t in ts) for z, ts in flat.items()}
    main = sorted(z for z in area if z > 0.2 and area[z] > 50)       # wings, gear, ...
    gear_top = max(z for z in main if z < letter_h - 0.5)            # highest below the letters
    floor_hi = min(main) - 0.2                                       # counterbore floors sit under the wings
    ring_pts = [(p[0], p[1]) for t in flat[round(gear_top, 2)] for p in t]
    cx = (min(p[0] for p in ring_pts) + max(p[0] for p in ring_pts)) / 2
    cy = (min(p[1] for p in ring_pts) + max(p[1] for p in ring_pts)) / 2
    # first guess: the inner edge is a full circle of vertices, so it is the most
    # populated radius (0.2 mm bins) around the gear's bounding-box centre
    hist = defaultdict(int)
    for x, y in ring_pts:
        d = math.hypot(x - cx, y - cy)
        if d > 3:
            hist[round(d / 0.2)] += 1
    # the outer edge is a circle too; take the smallest well-populated radius
    peak = max(hist.values())
    r = min(b for b, c in hist.items() if c >= 0.5 * peak) * 0.2
    for _ in range(4):
        band = [(x, y) for x, y in ring_pts if abs(math.hypot(x - cx, y - cy) - r) < 0.4]
        if len(band) < 8:
            raise RuntimeError(f"{path}: could not find the gear ring's inner edge")
        cx, cy, r = circle_fit(band)
    screws = floor_centres(flat, 0.2, floor_hi)
    if len(screws) != 2:
        raise RuntimeError(f"expected 2 counterbores in {path}, found {len(screws)}")
    (ax, ay), (bx, by) = sorted(screws, key=lambda s: (s[1], s[0]))
    axis = math.degrees(math.atan2(by - ay, bx - ax))
    # Handedness: on the SG90 design the hole line sits ~3 mm to the RIGHT of the
    # ring centre when looking along the wing axis. Mirrored exports would flip it.
    ux, uy = math.cos(math.radians(axis)), math.sin(math.radians(axis))
    mx, my = (ax + bx) / 2 - cx, (ay + by) / 2 - cy
    side = -uy * mx + ux * my          # >0 left of axis, <0 right
    if side > 0:
        raise RuntimeError(f"{path}: screw line is on the other side of the ring "
                           "(mirrored export?); letters would come out reversed")
    read_angle = axis - 180
    return {
        "emblem_letter_h": round(letter_h, 3), "gear_top": round(gear_top, 3),
        "ring_c": [round(cx, 3), round(cy, 3)], "ring_r": round(r + RING_R_PAD, 3),
        "read_angle": round((read_angle + 180) % 360 - 180, 2),
        "ring_fit_r": round(r, 3),
    }


# ---------------------------------------------------------------- openscad

def find_openscad(explicit):
    for c in ([explicit] if explicit else []) + [
            shutil.which("openscad"), r"C:\Program Files\OpenSCAD\openscad.exe",
            "/usr/bin/openscad", "/Applications/OpenSCAD.app/Contents/MacOS/OpenSCAD"]:
        if c and Path(c).exists():
            return c
    sys.exit("OpenSCAD not found; pass --openscad PATH")


def scad_value(v):
    if isinstance(v, str):
        return json.dumps(v)
    if isinstance(v, (list, tuple)):
        return "[" + ",".join(scad_value(x) for x in v) + "]"
    return repr(v)


def run_scad(openscad, scad, out, params, extra=()):
    cmd = [openscad, "-o", str(out)]
    for k, v in params.items():
        cmd += ["-D", f"{k}={scad_value(v)}"]
    cmd += list(extra) + [str(scad)]
    res = subprocess.run(cmd, capture_output=True, text=True)
    log = res.stdout + res.stderr
    if res.returncode != 0 or "ERROR" in log:
        raise RuntimeError(f"OpenSCAD failed for {scad.name}:\n{log}")
    m = re.findall(r"Volumes:\s+(\d+)", log)
    return int(m[-1]) if m else None


# ---------------------------------------------------------------- per servo

def build(openscad, servo_id, tmp):
    stl = PARTS / servo_id / "stl"
    hat, emb = stl / "Hat.stl", stl / "AiEmblem.stl"
    if not hat.exists() or not emb.exists():
        raise RuntimeError(f"missing {hat.name} or {emb.name} in {stl}")
    report = {}

    # hard hat
    h = measure_hat(hat)
    hp = {k: v for k, v in h.items()}
    hp["hat_stl"] = hat.as_posix()
    out = stl / "HardHat.stl"
    vols = run_scad(openscad, HERE / "hardhat.scad", out, hp)
    run_scad(openscad, HERE / "hardhat.scad", tmp / "interference.stl", {**hp, "part": "interference"})
    overlap = mesh_volume(load_tris(tmp / "interference.stl"))
    report["hardhat"] = {"measured": h, "solids": vols, "overlap_mm3": round(overlap, 4),
                         "ok": vols == 2 and overlap < 0.1}

    # emblem
    e = measure_emblem(emb)
    ep = {"emblem_stl": emb.as_posix(), "letter_h": e["emblem_letter_h"], "gear_top": e["gear_top"],
          "ring_c": e["ring_c"], "ring_r": e["ring_r"], "read_angle": e["read_angle"]}
    out = stl / "AiEmblemJT.stl"
    vols = run_scad(openscad, HERE / "emblem_jt.scad", out, ep)
    zmax = max(p[2] for t in load_tris(out) for p in t)
    joined = {}
    for g, name in ((0, "J"), (1, "T")):
        v = run_scad(openscad, HERE / "emblem_jt.scad", tmp / "flat.stl",
                     {**ep, "part": "flat", "glyph": g})
        joined[name] = v == 2
    report["emblem"] = {"measured": e, "solids": vols, "z_max": round(zmax, 3), "joined": joined,
                        "ok": vols == 2 and abs(zmax - e["emblem_letter_h"]) < 1e-3 and all(joined.values())}
    return report


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("servos", nargs="*", help="servo_id(s); default: every preset in servos.json")
    ap.add_argument("--openscad")
    args = ap.parse_args()
    openscad = find_openscad(args.openscad)
    ids = args.servos or [v["servo_id"] for v in json.loads(SERVOS_JSON.read_text()).values()]

    start = time.monotonic()
    results, failed = {}, []
    with tempfile.TemporaryDirectory() as td:
        for i, sid in enumerate(ids, 1):
            try:
                results[sid] = build(openscad, sid, Path(td))
            except Exception as exc:          # report and carry on with the other presets
                results[sid] = {"error": str(exc)}
            r = results[sid]
            ok = "error" not in r and r["hardhat"]["ok"] and r["emblem"]["ok"]
            if not ok:
                failed.append(sid)
            print(f"[{i}/{len(ids)}] {sid}: {'OK' if ok else 'PROBLEM'}  ({time.monotonic() - start:.0f}s)", flush=True)

    print()
    for sid, r in results.items():
        if "error" in r:
            print(f"{sid}: ERROR {r['error']}")
            continue
        h, e = r["hardhat"], r["emblem"]
        hm, em = h["measured"], e["measured"]
        print(f"{sid}:")
        print(f"  HardHat.stl     ridge y+/-{hm['ridge_y']} x[{hm['ridge_x0']},{hm['ridge_x1']}]  "
              f"shape_k {hm['shape_k']}  solids {h['solids']}  overlap {h['overlap_mm3']} mm3  "
              f"{'ok' if h['ok'] else 'CHECK'}")
        print(f"  AiEmblemJT.stl  ring r {em['ring_fit_r']} @ {em['ring_c']}  read {em['read_angle']}  "
              f"solids {e['solids']}  z {e['z_max']}  J {'joined' if e['joined']['J'] else 'FLOATS'}  "
              f"T {'joined' if e['joined']['T'] else 'FLOATS'}  {'ok' if e['ok'] else 'CHECK'}")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
