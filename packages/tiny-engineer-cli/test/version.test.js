import test from "node:test";
import assert from "node:assert/strict";
import { readFileSync } from "node:fs";
import { dirname, join } from "node:path";
import { fileURLToPath } from "node:url";

import { run } from "../src/cli.js";

const { version } = JSON.parse(
  readFileSync(join(dirname(fileURLToPath(import.meta.url)), "..", "package.json"), "utf8"),
);

/**
 * @param {string[]} argv
 */
async function captureVersion(argv) {
  /** @type {string[]} */
  const logs = [];
  const original = console.log;
  const previousExit = process.exitCode;
  console.log = (...args) => logs.push(args.join(" "));
  try {
    await run(argv);
    return logs;
  } finally {
    console.log = original;
    process.exitCode = previousExit;
  }
}

test("run --version prints package version", async () => {
  const logs = await captureVersion(["--version"]);
  assert.deepEqual(logs, [version]);
});

test("run -v prints package version", async () => {
  const logs = await captureVersion(["-v"]);
  assert.deepEqual(logs, [version]);
});

test("run -q --version prints package version", async () => {
  const logs = await captureVersion(["-q", "--version"]);
  assert.deepEqual(logs, [version]);
});
