import test from "node:test";
import assert from "node:assert/strict";

import { createOutput } from "../src/util/output.js";
import { createStyle } from "../src/util/style.js";
import { runPark } from "../src/commands/parity.js";

test("runPark --help prints usage", async () => {
  /** @type {string[]} */
  const logs = [];
  const original = console.log;
  console.log = (...args) => logs.push(args.join(" "));
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: () => {},
  });
  try {
    await runPark(out, ["--help"], false);
    assert.ok(logs.some((l) => l.includes("flash-safe")));
  } finally {
    console.log = original;
  }
});

test("runPark fails closed on invalid transport", async () => {
  /** @type {string[]} */
  const errs = [];
  const out = createOutput({
    quiet: false,
    style: createStyle({ color: false }),
    log: () => {},
    error: (m) => errs.push(String(m)),
  });
  await runPark(out, ["--transport", "ble"], false);
  assert.equal(process.exitCode, 1);
  assert.ok(errs.some((e) => e.includes("transport")));
  process.exitCode = 0;
});
