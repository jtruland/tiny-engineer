import preact from "@preact/preset-vite";
import { defineConfig } from "vitest/config";
import { viteSingleFile } from "vite-plugin-singlefile";

const robot = process.env.TINY_ENGINEER_URL || "http://tiny-engineer.local";
// Anchored so page routes such as /animations and /tests stay on the dev server.
const apiRoutes = [
  "^/auth([?]|$)",
  "^/health([?]|$)",
  "^/settings([/?]|$)",
  "^/anim([?]|$)",
  "^/play([/?]|$)",
  "^/test/",
  "^/setup/",
];

export default defineConfig({
  // One self-contained page: the firmware embeds it gzipped and serves it as-is.
  plugins: [preact({ reactAliasesEnabled: false }), viteSingleFile()],
  test: {
    environment: "jsdom",
    unstubGlobals: true,
  },
  server: {
    host: "127.0.0.1",
    proxy: Object.fromEntries(apiRoutes.map((route) => [route, robot])),
  },
});
