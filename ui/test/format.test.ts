import { describe, expect, it } from "vitest";
import { formatBytes, formatDuration } from "../src/lib/format";

describe("formatDuration", () => {
  it("shows the two largest units as the duration grows", () => {
    expect(formatDuration(59_999)).toBe("59 s");
    expect(formatDuration(61_000)).toBe("1 min 1 s");
    expect(formatDuration(3_660_000)).toBe("1 h 1 min");
    expect(formatDuration(90_000_000)).toBe("1 d 1 h");
  });
});

describe("formatBytes", () => {
  it("switches units at 1 KB and 1 MB", () => {
    expect(formatBytes(1023)).toBe("1023 B");
    expect(formatBytes(159_744)).toBe("156 KB");
    expect(formatBytes(1_572_864)).toBe("1.5 MB");
  });
});
