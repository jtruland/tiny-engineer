// Tiny Engineer desk emblem, with "JT" in place of "AI" (SG90 preset).
//
// Built on the stock part (jamro/tiny-engineer 3d_models/parts/sg90/stl/AiEmblem.stl)
// so the wings, gear and M2 mount holes stay the original geometry, following the
// constraints in jamro/tiny-engineer#118:
//   - only the centre symbol changes;
//   - the layer heights stay the same: letters 4.5, gear 3.0, wings 1.5 mm;
//   - the letters join the gear ring (no floating islands), checked at render time;
//   - one manifold solid, printed flat in the stock orientation with no supports.
//
// Measured from the stock mesh:
//   gear ring inner circle: centre (-0.055, -0.136), r = 10.03 (fit over 333 vertices)
//   only the stock letters stand above z = 3 (gear top); their tops are at z = 4.5
//
// The J and T are drawn from strokes measured off Rockwell Extra Bold (size 12.5,
// 0.3 mm outline offset), so their weights match the font but the layout fits the
// ring: two distinct slab-serif letters, each with its own top bar and downward
// serifs, joined by a narrow connector. Units are mm in the TEXT frame: x reads
// left to right, y is up, origin at the ring centre.

/* [Strokes, measured from the font] */
bar_y     = [4.21, 6.33];  // top bar band (T crossbar)
drop_w    = 2.06;          // downward serif width
drop_bot  = 1.21;          // downward serif bottom
stem_w    = 2.38;          // stem width
foot_y    = [-6.06, -3.99];// T foot band; also the J bowl bottom
foot_half = 2.2;           // T foot half-width (font: 2.71)
conn_top  = true;          // connectors flush with the bar tops, so the letters hang from one line (false = centred on the bars)

/* [Layout] */
conn_t    = 1.0;   // connector thickness: narrower than the 2.12 bars, >= 2 extrusion widths
// Both letters share one top-bar shape, symmetric about the stem: half the stem, a
// 0.8 mm gap, then the downward serif -> half-width 4.05.
bar_half  = stem_w / 2 + 0.8 + drop_w;     // T bar half-width (4.05); fixed so the layout stays put
t_stem_w  = 2.0;   // T stem
j_stem_w  = t_stem_w;   // J stem and bowl stroke, matched to the T
t_foot_h  = 1.5;   // T foot height (font: 2.07)
t_drop_w  = 1.6;   // T downward serif width (font: 2.06 -- read as a comb next to the stem)
t_drop_d  = 1.8;   // T downward serif depth below the bar (font: 3.0)
// The J's top bar is a plain symmetric slab: no downward serifs (set j_drop_w > 0 to
// bring them back, e.g. 1.6 wide x 1.8 deep).
j_drop_w  = 0;
j_drop_d  = 1.8;
j_half    = stem_w / 2 + 0.8 + 1.6;        // J bar half-width (3.59), kept from the serif version
link_gap  = 1.2;   // J bar end to T bar end (unscaled)
// J style: false = no top bar -- the stem rises to the top of the ring (fusing into
// it) and the crossline runs straight through it from the ring to the T.
j_top_bar = false;
// Bar-less J stem top, actual mm from the ring centre (not scaled). 0 = run on into
// the ring as a joint. 8.0 stops it clear of the ring (ring is ~9.6 above the stem)
// and still ~1.3 above the T's top corner (6.67).
j_stem_top = 8.0;
j_x       = -2.4;    // J stem centre (moved toward the T to make room for the bigger bowl)
t_x       = 4.19;    // T stem centre (where the earlier centred layout put it)
art_scale = 0.86;  // letters only (connectors keep conn_t); largest that passes the near_ring check (0.88 fails)
j_loop_ro = 5.2;   // J bowl outer half-width (largest that passes the ring check at j_x -2.4)
j_loop_ry = 4.0;   // J bowl outer half-height (less than j_loop_ro = flatter bowl, longer stem)
j_lift    = 0;       // raise the J bowl bottom above the T foot line (the ring is wider higher up)
j_end_angle = 240; // where the hook stops, deg CCW from +x (180 = level with the bowl centre, parallel to the stem)

/* [Fit] */
letter_h  = 4.5;
gear_top  = 3.0;
ring_c    = [3.986, 2.353];   // this folder's AiEmblem (Fusion re-export): ring fit over the inner edge, r 10.017
ring_r    = 10.03;
overlap   = 0.6;   // bar ends run this far into the ring so they fuse to it
min_gap   = 0.8;   // anything closer than this to the ring (and not fused) gets flagged

/* [Orientation] */
// Baseline runs along the wings. Verified by overlaying "AI" in Rockwell Extra Bold on
// the stock lettering (part="compare"): -107.6 with no mirror lines up.
read_angle = -90;   // wing axis is +y in this folder's AiEmblem (upstream's was 72.43 deg -> -107.6)

part = "emblem";   // [emblem, upright, near_ring, contact, compare]
$fn = 128;

module stock() { import("../../parts/sg90/stl/AiEmblem.stl", convexity = 6); }

// Top bar with a downward serif at each end, symmetric about stem centre x.
module top_bar(x, h, dw, db) {
    translate([x - h, bar_y[0]]) square([2 * h, bar_y[1] - bar_y[0]]);
    if (dw > 0) for (e = [x - h, x + h - dw])
        translate([e, db]) square([dw, bar_y[0] - db + 0.01]);
}

module drawn_j() {
    j_stem_r = j_x + j_stem_w / 2;
    rx = j_loop_ro; ry = j_loop_ry;
    xc = j_stem_r - rx;                 // bowl centre; the stem is its right side
    yc = foot_y[0] + j_lift + ry;
    if (j_top_bar) top_bar(j_x, j_half, j_drop_w, bar_y[0] - j_drop_d);
    // stem: to the bar top, or (no bar) on past the ring, which clips it into a joint
    translate([j_stem_r - j_stem_w, yc]) square([j_stem_w, (j_top_bar ? bar_y[1] : (j_stem_top > 0 ? j_stem_top / art_scale : 30)) - yc]);
    // bowl: elliptical ring (constant stroke j_stem_w) from the stem (0 deg) down round
    // the bottom, stopping at j_end_angle -- short of the left side, so the hook end
    // is cut rather than turning up parallel to the stem.
    translate([xc, yc]) intersection() {
        difference() {
            scale([rx, ry]) circle(r = 1);
            scale([rx - j_stem_w, ry - j_stem_w]) circle(r = 1);
        }
        R = max(rx, ry) + 1;
        polygon(concat([[0, 0]], [for (a = [j_end_angle : 5 : 360]) R * [cos(a), sin(a)]], [R * [1, 0]]));
    }
}

// The T is enlarged about its bottom-left (bar's left end, foot line) so the gap to the
// J and the foot line stay put while its top-right bar corner reaches the ring.
// 1.0904 puts that corner exactly on the ring; 1.115 runs it 0.3 mm in so it fuses.
t_scale = 1.115;
module drawn_t() {
    a = [t_x - bar_half, foot_y[0]];
    translate(a) scale(t_scale) translate(-a) {
        top_bar(t_x, bar_half, t_drop_w, bar_y[0] - t_drop_d);
        // carry the bar on into the ring so its end is a clean joint, not a corner
        // touching the ring with a tapering hairline gap under it
        translate([t_x + bar_half - 0.01, bar_y[0]]) square([20, bar_y[1] - bar_y[0]]);
        // stem and foot
        translate([t_x - t_stem_w / 2, foot_y[0]]) square([t_stem_w, bar_y[1] - foot_y[0]]);
        translate([t_x - foot_half, foot_y[0]]) square([2 * foot_half, t_foot_h]);
    }
}

// Narrow links (absolute conn_t, not scaled): J to T, and each bar's outer end to the ring.
// Crossline style: "wing_gaps" = two ring-to-ring lines in line with the two open
// channels between the wing stripes (measured off the stock mesh, text frame, absolute
// mm: stripes y -5.95..-3.0 / -1.45..1.5 / 3.05..6.0, so channels -3.0..-1.45 and
// 1.5..3.05). "serif" = the earlier single line along the bar tops.
cross_style = "serif";
wing_gaps   = [[-3.0, -1.45], [1.5, 3.05]];
wing_line_t = 1.0;   // line thickness, centred in each wing channel (the channels are 1.55)
module connectors() {
    if (cross_style == "wing_gaps")
        for (g = wing_gaps) translate([-20, (g[0] + g[1]) / 2 - wing_line_t / 2]) square([40, wing_line_t]);
    else serif_connectors();
}
module serif_connectors() {
    s = art_scale;
    mid = conn_top ? s * bar_y[1] - conn_t / 2 : s * (bar_y[0] + bar_y[1]) / 2;
    spans = j_top_bar
        ? [[s * (j_x + j_half), s * (t_x - bar_half)],   // J -> T
           [-20, s * (j_x - j_half)],                    // ring -> J
           [s * (t_x + bar_half), 20]]                   // T -> ring
        : [[-20, s * (t_x - bar_half)],                  // ring -> through the J stem -> T
           [s * (t_x + bar_half), 20]];                  // T -> ring
    for (span = spans)
        translate([span[0] - 0.01, mid - conn_t / 2]) square([span[1] - span[0] + 0.02, conn_t]);
}

module letters_text_frame() { scale(art_scale) { drawn_j(); drawn_t(); } connectors(); }

module to_part_frame() { translate(ring_c) rotate(read_angle) children(); }

module letters2d() { to_part_frame() letters_text_frame(); }

module clipped2d() { intersection() { letters2d(); translate(ring_c) circle(r = ring_r + overlap); } }

// Letters stand letter_h (4.5); the crossline links stand conn_h, flush with the gear top.
conn_h = gear_top;
module clip_to_ring() { intersection() { children(); translate(ring_c) circle(r = ring_r + overlap); } }
// Morphological closing: fills any gap narrower than 2 * fill_d (letter-to-ring wedges
// where a bar meets the ring at a shallow angle, near-touches) with a small fillet, and
// leaves wider gaps untouched. 2 * 0.4 = min_gap.
fill_d = min_gap / 2;
module ring_band() { translate(ring_c) difference() { circle(r = ring_r + overlap + 1); circle(r = ring_r); } }
module closed2d() { offset(r = -fill_d) offset(r = fill_d) children(); }
module letters_part2d() { to_part_frame() scale(art_scale) { drawn_j(); drawn_t(); } }
// Close against the ring, keep only what lands inside the ring's inner circle (the
// fillets), then add back the letters' own overlap into the ring for the joints.
module with_fillets() {
    union() {
        intersection() {
            closed2d() { children(); ring_band(); }
            // only near the ring: the letters' own inside corners stay sharp
            translate(ring_c) difference() { circle(r = ring_r - 0.01); circle(r = ring_r - 1.5); }  // -0.01: coincident circles broke the mesh
        }
        clip_to_ring() children();
    }
}
// clean2d: a tiny grow-then-shrink merges edges that only touch, which otherwise
// extrude to a non-closed mesh that CGAL silently drops (the letters vanished).
module clean2d() { offset(delta = -0.005) offset(delta = 0.005) children(); }
module top_layer2d() { clean2d() with_fillets() letters_part2d(); }
module low_layer2d() { clean2d() with_fillets() { letters_part2d(); to_part_frame() connectors(); } }
module new_symbol() {
    linear_extrude(height = letter_h) top_layer2d();
    linear_extrude(height = conn_h)   low_layer2d();
}

// 2D, on the FINAL (filleted) layers: material within min_gap of the ring that is not
// part of, or within 1.5 mm of, an actual joint into the ring. Non-empty = a near-miss
// that will print as a hairline gap.
module near_miss_of() {
    difference() {
        intersection() {
            children();
            translate(ring_c) difference() { circle(r = ring_r); circle(r = ring_r - min_gap); }
        }
        offset(r = 1.5) intersection() {
            children();
            translate(ring_c) difference() { circle(r = ring_r + overlap); circle(r = ring_r + 0.05); }
        }
    }
}
if (part == "near_miss") { near_miss_of() top_layer2d(); near_miss_of() low_layer2d(); }

module stock_without_symbol() {
    difference() {
        stock();
        translate([-100, -100, gear_top]) cube([200, 200, 20]);                        // stock letters' upper part
        translate([ring_c[0], ring_c[1], -1]) cylinder(r = ring_r - 0.01, h = 20);  // and their lower part
    }
}

module emblem() { union() { stock_without_symbol(); new_symbol(); } }

if (part == "emblem") emblem();

// Turned so the text reads upright, for judging the look.
if (part == "upright") rotate(-read_angle) translate(-ring_c) emblem();

// 2D: letter area within min_gap of the ring that is NOT part of a real joint.
// Non-empty = a hairline crack or near-touch that will print badly.
if (part == "near_ring")
    difference() {
        intersection() {
            letters2d();
            translate(ring_c) difference() { circle(r = ring_r); circle(r = ring_r - min_gap); }
        }
        // the ring connectors are meant to reach the ring; exclude just them (plus a hair)
        to_part_frame() offset(delta = 0.05) ring_connectors();
    }
module ring_connectors() {
    s = art_scale; mid = conn_top ? s * bar_y[1] - conn_t / 2 : s * (bar_y[0] + bar_y[1]) / 2;
    if (cross_style == "wing_gaps") connectors();
    else for (span = [[-20, s * (j_top_bar ? j_x - j_half : t_x - bar_half)], [s * (t_x + bar_half), 20]])
        translate([span[0], mid - conn_t / 2]) square([span[1] - span[0], conn_t]);
    // the bar-less J stem is meant to fuse into the top of the ring
    if (!j_top_bar && j_stem_top <= 0)
        translate([s * (j_x - j_stem_w / 2), s * bar_y[0]]) square([s * j_stem_w, 30]);
}

// 2D: where the letters fuse into the ring (for measuring joint width).
if (part == "contact")
    intersection() {
        letters2d();
        translate(ring_c) difference() { circle(r = ring_r + overlap); circle(r = ring_r); }
    }

// Orientation proof: font "AI" over the stock lettering.
if (part == "compare") {
    color("royalblue") stock();
    color("orange") translate([0, 0, 6]) linear_extrude(0.5)
        translate(ring_c) rotate(read_angle)
            text("AI", size = 13, font = "Rockwell:style=Extra Bold", halign = "center", valign = "center");
}
