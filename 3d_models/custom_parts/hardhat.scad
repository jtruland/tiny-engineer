// Tiny Engineer — construction hard hat that keys onto the stock `Hat`.
//
// Every dimension that depends on the Hat is a parameter. build_custom_parts.py
// measures each parts/{servo_id}/stl/Hat.stl (the TinyEngineer Tools export for that
// servos.json preset) and passes them with -D, so the hard hat follows the preset.
// The defaults below are the SG90 Fusion export, measured 2026-11-22:
//   - Hat top surface .......... z = hat_top
//   - Vent ridge ............... top x[ridge_x0, ridge_x1] y ±ridge_y, ridge_h tall;
//                                its filleted base ends reach pocket_x0 / pocket_x1
//   - Screw counterbores ....... screws[] (heads sit recessed, floor at z = 1)
//   - Front (OLED side) ........ +x
//
// Modelled with its underside at z = 0 (print brim-down, no supports) and placed on
// the Hat by translate([0, 0, hat_top]) — see `fit_check`.

/* [Stock part] */
hat_stl    = "../parts/sg90/stl/Hat.stl";
hat_top    = 3.0;

/* [Fit] */
fit_y      = 0.15;  // per-side clearance on the ridge's long walls (press-fit). 0.10 tighter, 0.25 looser
ridge_h    = 2.0;   // vent ridge height above Hat top
pocket_extra_h = 0.3;   // pocket deeper than ridge so the brim seats on the Hat, not the ridge
mouth_chamfer  = 0.8;   // clears the ridge's base fillet

// Ridge footprint (top of ridge) and its filleted base ends
ridge_x0 = -28.69; ridge_x1 = 8.75; ridge_y = 10.45;
pocket_x0 = -32.2; pocket_x1 = 11.4;
screws   = [[-30.65, -13.45], [-0.10, 13.45]];

/* [Shape] */
// The look was designed on the SG90 Hat (54.1 mm wide); shape_k scales it to others.
shape_k    = 1.0;
center_x   = -10.2; // middle of the Hat front-to-back
dome_a     = 27  * shape_k;  // dome half-length (x)
dome_b     = 20  * shape_k;  // dome half-width  (y)
dome_h     = 14  * shape_k;  // dome height above brim underside
brim_a     = 29  * shape_k;  // brim half-length
brim_b     = 23  * shape_k;  // brim half-width
brim_t     = 1.6;            // brim thickness
peak       = 5   * shape_k;  // extra brim reach at the front (+x)
crest_w    = 5   * shape_k;  // centre crest rib width
crest_h    = 1.2;            // crest rib proud of dome

/* [Render] */
part = "hardhat";   // [hardhat, fit_check, interference]
$fn = 96;

module dome(a, b, h) {
    intersection() {
        scale([a, b, h]) sphere(r = 1);
        translate([-a, -b, 0]) cube([2 * a, 2 * b, h]);
    }
}

module brim() {
    hull() {
        translate([center_x, 0, 0]) scale([brim_a, brim_b, 1]) cylinder(r = 1, h = brim_t);
        translate([center_x + peak, 0, 0]) scale([brim_a, brim_b * 0.82, 1]) cylinder(r = 1, h = brim_t);
    }
}

module crest() {
    intersection() {
        translate([center_x, 0, 0]) dome(dome_a + crest_h, dome_b + crest_h, dome_h + crest_h);
        translate([center_x - dome_a - 5, -crest_w / 2, 0]) cube([2 * dome_a + 10, crest_w, dome_h + 5]);
    }
}

module ridge_pocket() {
    d = ridge_h + pocket_extra_h;
    w = ridge_y + fit_y;
    // straight-walled key that grips the ridge's vertical long sides
    translate([pocket_x0, -w, -1]) cube([pocket_x1 - pocket_x0, 2 * w, d + 1]);
    // chamfered mouth for the base fillet
    hull() {
        translate([pocket_x0 - mouth_chamfer, -w - mouth_chamfer, -1]) cube([pocket_x1 - pocket_x0 + 2 * mouth_chamfer, 2 * (w + mouth_chamfer), 1]);
        translate([pocket_x0, -w, 0]) cube([pocket_x1 - pocket_x0, 2 * w, mouth_chamfer]);
    }
}

module screw_reliefs() {
    // heads are recessed in the Hat already; shallow relief in case a head stands proud
    for (p = screws)
        translate([p[0], p[1], -1]) cylinder(d = 5.0, h = 1.6);
}

// The ridge pocket is long and nearly reaches the dome's elliptical edge, so its
// corners would cut through the brim. Hull the dome with a low rounded band around
// the pocket so there is always >= band_wall of material around it.
band_wall = 1.6;
band_h    = 4.0;
module crown() {
    w = ridge_y + fit_y + mouth_chamfer + band_wall;
    x0 = pocket_x0 - mouth_chamfer - band_wall;
    x1 = pocket_x1 + mouth_chamfer + band_wall;
    r = 3;
    hull() {
        translate([center_x, 0, 0]) dome(dome_a, dome_b, dome_h);
        for (x = [x0 + r, x1 - r], y = [-w + r, w - r])
            translate([x, y, 0]) cylinder(r = r, h = band_h);
    }
}

module hardhat() {
    difference() {
        union() {
            brim();
            crown();
            crest();
        }
        ridge_pocket();
        screw_reliefs();
    }
}

module stock_hat() { import(hat_stl); }

if (part == "hardhat") hardhat();
if (part == "fit_check") { color("royalblue") stock_hat(); color("gold") translate([0, 0, hat_top]) hardhat(); }
if (part == "interference") intersection() { stock_hat(); translate([0, 0, hat_top]) hardhat(); }
