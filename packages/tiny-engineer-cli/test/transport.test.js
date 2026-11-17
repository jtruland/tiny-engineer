import test from "node:test";
import assert from "node:assert/strict";
import { mkdtempSync, rmSync, writeFileSync } from "node:fs";
import { tmpdir } from "node:os";
import { join } from "node:path";

import {
  getHttpAuthToken,
  getToken,
  loadDotEnv,
  resolveTransport,
} from "../src/config/env.js";

test("resolveTransport defaults to http", () => {
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  try {
    assert.deepEqual(resolveTransport(undefined), { transport: "http" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("resolveTransport prefers flag over env", () => {
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_TRANSPORT = "serial";
  try {
    assert.deepEqual(resolveTransport("http"), { transport: "http" });
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("resolveTransport rejects invalid values", () => {
  assert.equal(resolveTransport("ble").error?.includes("invalid --transport"), true);
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_TRANSPORT = "ble";
  try {
    assert.equal(resolveTransport(undefined).error?.includes("TINY_ENGINEER_TRANSPORT"), true);
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
  }
});

test("project .env transport is loaded for later commands", () => {
  const dir = mkdtempSync(join(tmpdir(), "te-transport-"));
  const prev = process.env.TINY_ENGINEER_TRANSPORT;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  try {
    writeFileSync(join(dir, ".env"), "TINY_ENGINEER_TRANSPORT=serial\n", "utf8");
    loadDotEnv(dir);
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prev === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prev;
    rmSync(dir, { recursive: true, force: true });
  }
});

test("project .env overrides user env transport", () => {
  const project = mkdtempSync(join(tmpdir(), "te-proj-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prevT = process.env.TINY_ENGINEER_TRANSPORT;
  const prevC = process.env.TINY_ENGINEER_CONFIG_DIR;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  try {
    writeFileSync(join(cfg, "env"), "TINY_ENGINEER_TRANSPORT=http\n", "utf8");
    writeFileSync(join(project, ".env"), "TINY_ENGINEER_TRANSPORT=serial\n", "utf8");
    loadDotEnv(project);
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prevT === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prevT;
    if (prevC === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevC;
    rmSync(project, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
  }
});

test("user env fills when project key absent", () => {
  const project = mkdtempSync(join(tmpdir(), "te-proj-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prevT = process.env.TINY_ENGINEER_TRANSPORT;
  const prevC = process.env.TINY_ENGINEER_CONFIG_DIR;
  delete process.env.TINY_ENGINEER_TRANSPORT;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  try {
    writeFileSync(join(cfg, "env"), "TINY_ENGINEER_TRANSPORT=serial\n", "utf8");
    writeFileSync(join(project, ".env"), "FOO=1\n", "utf8");
    loadDotEnv(project);
    assert.deepEqual(resolveTransport(undefined), { transport: "serial" });
  } finally {
    if (prevT === undefined) delete process.env.TINY_ENGINEER_TRANSPORT;
    else process.env.TINY_ENGINEER_TRANSPORT = prevT;
    if (prevC === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevC;
    rmSync(project, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
  }
});

test("getHttpAuthToken blocks user token against project-only URL", () => {
  const project = mkdtempSync(join(tmpdir(), "te-proj-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prevU = process.env.TINY_ENGINEER_URL;
  const prevTok = process.env.TINY_ENGINEER_TOKEN;
  const prevC = process.env.TINY_ENGINEER_CONFIG_DIR;
  delete process.env.TINY_ENGINEER_URL;
  delete process.env.TINY_ENGINEER_TOKEN;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  /** @type {string[]} */
  const stderrChunks = [];
  const origWrite = process.stderr.write.bind(process.stderr);
  process.stderr.write = /** @type {typeof process.stderr.write} */ (
    (chunk, ...args) => {
      stderrChunks.push(String(chunk));
      return origWrite(chunk, ...args);
    }
  );
  try {
    writeFileSync(join(project, ".env"), "TINY_ENGINEER_URL=http://project.local\n", "utf8");
    writeFileSync(join(cfg, "env"), "TINY_ENGINEER_TOKEN=user-secret\n", "utf8");
    loadDotEnv(project);
    assert.equal(getToken(), "user-secret");
    assert.equal(getHttpAuthToken(), null);
    const warn = stderrChunks.join("");
    assert.ok(warn.includes("withholding Authorization"));
    assert.ok(warn.includes("TINY_ENGINEER_URL"));
    assert.ok(warn.includes("TINY_ENGINEER_TOKEN"));
    assert.ok(!warn.includes("user-secret"));
    assert.equal(getHttpAuthToken({ urlOverride: true }), "user-secret");
  } finally {
    process.stderr.write = origWrite;
    if (prevU === undefined) delete process.env.TINY_ENGINEER_URL;
    else process.env.TINY_ENGINEER_URL = prevU;
    if (prevTok === undefined) delete process.env.TINY_ENGINEER_TOKEN;
    else process.env.TINY_ENGINEER_TOKEN = prevTok;
    if (prevC === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevC;
    rmSync(project, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
  }
});

test("getHttpAuthToken allows process URL with user token", () => {
  const project = mkdtempSync(join(tmpdir(), "te-proj-"));
  const cfg = mkdtempSync(join(tmpdir(), "te-ucfg-"));
  const prevU = process.env.TINY_ENGINEER_URL;
  const prevTok = process.env.TINY_ENGINEER_TOKEN;
  const prevC = process.env.TINY_ENGINEER_CONFIG_DIR;
  process.env.TINY_ENGINEER_URL = "http://explicit.local";
  delete process.env.TINY_ENGINEER_TOKEN;
  process.env.TINY_ENGINEER_CONFIG_DIR = cfg;
  try {
    writeFileSync(join(project, ".env"), "TINY_ENGINEER_URL=http://project.local\n", "utf8");
    writeFileSync(join(cfg, "env"), "TINY_ENGINEER_TOKEN=user-secret\n", "utf8");
    loadDotEnv(project);
    assert.equal(getHttpAuthToken(), "user-secret");
  } finally {
    if (prevU === undefined) delete process.env.TINY_ENGINEER_URL;
    else process.env.TINY_ENGINEER_URL = prevU;
    if (prevTok === undefined) delete process.env.TINY_ENGINEER_TOKEN;
    else process.env.TINY_ENGINEER_TOKEN = prevTok;
    if (prevC === undefined) delete process.env.TINY_ENGINEER_CONFIG_DIR;
    else process.env.TINY_ENGINEER_CONFIG_DIR = prevC;
    rmSync(project, { recursive: true, force: true });
    rmSync(cfg, { recursive: true, force: true });
  }
});
