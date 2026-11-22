# Custom parts

Personal add-ons built on top of the stock printed parts, for every servo preset.

| Output (per `parts/{servo_id}/stl/`) | Source | What it is |
| --- | --- | --- |
| `HardHat.stl` | `hardhat.scad` | Construction hard hat that press-fits over the stock `Hat`'s vent ridge (0.15 mm/side). Print brim-down, no supports. |
| `AiEmblemJT.stl` | `emblem_jt.scad` | Desk emblem with "JT" (Georgia Bold) in place of "AI". Wings, gear and M2 mounts are the stock part's. |

## Rebuilding after a Parts Exporter run

The stock parts change size with the `servos.json` preset, and with any parameter you change in Fusion. After exporting with **Tiny Engineer Parts Exporter**, rebuild:

```sh
python 3d_models/custom_parts/build_custom_parts.py            # every preset in servos.json
python 3d_models/custom_parts/build_custom_parts.py sg90       # just one
```

For each preset it measures that folder's `Hat.stl` (ridge, screw counterbores, size) and `AiEmblem.stl` (gear ring centre and radius, wing axis, layer heights), renders through OpenSCAD with those values, and checks the result:

- **hard hat**: a single solid that only touches the Hat at its seating face (overlap < 0.1 mm³)
- **emblem**: a single solid, letters at the stock letter height, and J and T each joined to the gear ring on their own

It exits non-zero and prints `CHECK` / `ERROR` against any preset that fails. Needs Python 3 (standard library only) and OpenSCAD.

## Tuning

- Press fit too tight or loose: `fit_y` in `hardhat.scad` (0.10 tighter, 0.25 looser).
- Hard hat look (dome, brim, crest): the `[Shape]` section. It was designed on the SG90 Hat and scales with `shape_k` (Hat width ÷ 54.1 mm).
- Emblem lettering: font, size and letter nudges at the top of `emblem_jt.scad`. The runner-up setting (Bernard MT Condensed 13) is noted in its header.

## Other files

- `alternates/emblem_jt_handdrawn.scad`: an earlier hand-drawn JT, SG90 only. Its output is `parts/sg90/stl/alternates/AiEmblemJT_HandDrawn.stl`. The SG90 Bernard version is in the same folder.
- `previews/`: renders from the design process.
