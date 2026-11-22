// "JT" set in a real font, joined to the stock emblem's gear ring (replaces "AI").
//
// CHOSEN DESIGN (defaults below): Georgia Bold 11, join "touch". The T's bar end runs
// into the ring as-is; the J is nudged 0.56 mm toward its closest ring point so its
// ball terminal runs 0.5 mm in. J-T clearance 0.80 mm. Letters stand letter_h and
// sit on top of the ring where they cross it; joint fillets are at ring height.
// Runner-up: Bernard MT Condensed 13, split_x -1.34, j_shift [-0.15,0], t_shift [0.15,0].
//
// Everything that depends on the stock AiEmblem is a parameter. build_custom_parts.py
// measures each parts/{servo_id}/stl/AiEmblem.stl (ring centre + radius, wing axis,
// layer heights) and passes them with -D. The letter layout was designed on a ring
// of base_ring_r and is scaled by k = ring_r / base_ring_r for other presets.
// Defaults = the SG90 Fusion export, measured 2026-11-22.
//
// join:
//   "touch"    - letters scaled up until their extremities run into the ring
//   "line"     - letters sit inside the ring; one thin line (flush with the ring top)
//                runs ring to ring through both letters at line_y
//   "baseline" - letters sit on a thin bar along their baseline that runs ring to ring

font      = "Georgia:style=Bold";
label     = "JT";
size      = 11;
spacing   = 1.0;
offs      = [0, 0];        // text-frame nudge [right, up], mm
join      = "touch";       // [touch, line, baseline]
line_y    = 0;             // text-frame y of the line (join = "line")
line_t    = 1.0;           // line thickness
glyph     = -1;            // -1 all; 0/1 = only that letter (connectivity checks)

/* [Stock part, measured] */
emblem_stl = "../parts/sg90/stl/AiEmblem.stl";
letter_h  = 4.5;           // stock letter top
gear_top  = 3.0;           // stock gear ring top
ring_c    = [3.986, 2.353];   // gear ring inner circle centre
ring_r    = 10.03;            // gear ring inner radius (fit 10.017; 10.03 keeps the cut clear of it)
read_angle = -90;          // wing axis angle - 180 (SG90 Fusion export: wing axis +y)
base_ring_r = 10.03;       // ring the letter layout was designed on
k         = ring_r / base_ring_r;

line_h    = gear_top;      // lines/joints flush with the gear top
overlap   = 0.6;
fill_d    = 0.4;
part      = "emblem";      // [emblem, upright, flat]
$fn = 128;

module stock() { import(emblem_stl, convexity = 6); }
module stock_without_symbol() {
    difference() {
        stock();
        translate([-100, -100, gear_top]) cube([200, 200, 20]);
        translate([ring_c[0], ring_c[1], -1]) cylinder(r = ring_r - 0.01, h = 20);
    }
}
module to_part_frame() { translate(ring_c) rotate(read_angle) children(); }
module clip_to_ring() { intersection() { children(); translate(ring_c) circle(r = ring_r + overlap); } }

// Letters in the text frame, centred on the ring centre. The word is split at split_x
// (in the white gap between J and T) so each letter can be nudged on its own
// (j_shift / t_shift) and isolated for a connectivity check (glyph = 0 / 1).
split_x = -0.71;              // Georgia Bold 11: J right edge -0.85, T left edge -0.57
j_shift = [-0.517, -0.216];   // 0.56 mm along the J's closest-to-ring direction
t_shift = [0, 0];
module word() { text(label, size = size, font = font, halign = "center", valign = "center", spacing = spacing); }
module letters_tf() {
    translate(offs) scale(k) {
        if (glyph != 1) translate(j_shift) intersection() { word(); translate([-40 + split_x, -40]) square([40, 80]); }
        if (glyph != 0) translate(t_shift) intersection() { word(); translate([split_x, -40]) square([40, 80]); }
    }
}

module lines_tf() {
    if (join == "line")     translate([-20, line_y - line_t / 2]) square([40, line_t]);
    if (join == "baseline") translate([-20, line_y - line_t / 2]) square([40, line_t]);
}

// fillets where something meets the ring at a shallow angle (gaps < 2*fill_d)
module ring_band() { translate(ring_c) difference() { circle(r = ring_r + overlap + 1); circle(r = ring_r); } }
module with_fillets() {
    union() {
        intersection() {
            offset(r = -fill_d) offset(r = fill_d) { children(); ring_band(); }
            translate(ring_c) difference() { circle(r = ring_r - 0.01); circle(r = ring_r - 1.5); }
        }
        clip_to_ring() children();
    }
}
module clean2d() { offset(delta = -0.005) offset(delta = 0.005) children(); }

// letters_over_ring = true: each letter keeps its full drawn shape at letter_h, so
// where it crosses the ring it stands proud ON TOP of the ring (like the stock "AI").
// false: the 4.5 mm material stops at the ring's inner edge and the joints sit at
// ring height. The joint fillets stay at ring height (line_h = gear top) either way.
letters_over_ring = true;
ring_outer_r = 13.0 * k;   // gear ring top face reaches ~13 mm (teeth beyond); letters never get near it
module top2d() {
    clean2d() intersection() {
        to_part_frame() letters_tf();
        translate(ring_c) circle(r = letters_over_ring ? ring_outer_r : ring_r - 0.01);
    }
}
module low2d() { clean2d() with_fillets() to_part_frame() { letters_tf(); lines_tf(); } }

module symbol3d() {
    linear_extrude(height = letter_h) top2d();
    linear_extrude(height = line_h) low2d();
}
module emblem() { union() { stock_without_symbol(); symbol3d(); } }

if (part == "emblem")  emblem();
if (part == "upright") rotate(-read_angle) translate(-ring_c) emblem();
// flat: ring + one glyph (+ lines) as a single 3D layer, for "is this letter joined?"
if (part == "flat") difference() { linear_extrude(1) union() {
    translate(ring_c) difference() { circle(r = ring_r + 3); circle(r = ring_r); }
    low2d();
} translate([200, 200, 0]) cube(1); }   // the far cube only forces a CGAL pass (volume count)
