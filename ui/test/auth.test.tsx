import { screen, waitFor } from "@testing-library/preact";
import { describe, expect, it } from "vitest";
import { fakeRobot, renderApp } from "./robot";

describe("access token gate", () => {
  it("asks for a token when the robot requires one and none is stored", async () => {
    fakeRobot({ authRequired: true, token: "secret" });
    renderApp("/config");

    await screen.findByRole("heading", { name: "Access token required" });
    expect(screen.queryByRole("navigation")).toBeNull();
  });

  it("rejects a wrong token without remembering it", async () => {
    fakeRobot({ authRequired: true, token: "secret" });
    const { user } = renderApp("/config");

    await user.type(await screen.findByLabelText("Access token"), "wrong{Enter}");

    expect((await screen.findByRole("alert")).textContent).toBe("Invalid access token.");
    expect(sessionStorage.length).toBe(0);
  });

  it("says when the robot cannot be reached instead of blaming the token", async () => {
    const robot = fakeRobot({ authRequired: true, token: "secret" });
    const { user } = renderApp("/config");
    const field = await screen.findByLabelText("Access token");

    robot.failing.add("/settings");
    await user.type(field, "secret{Enter}");

    expect((await screen.findByRole("alert")).textContent).toBe(
      "Could not reach the robot. Try again.",
    );
  });

  it("unlocks with the right token and sends it on later requests", async () => {
    const robot = fakeRobot({ authRequired: true, token: "secret" });
    const { user } = renderApp("/config");

    await user.type(await screen.findByLabelText("Access token"), "secret{Enter}");

    await screen.findByRole("heading", { name: "Config" });
    await waitFor(() => expect(robot.requests.at(-1)?.authorization).toBe("Bearer secret"));
  });

  it("skips the gate when a stored token is still accepted", async () => {
    fakeRobot({ authRequired: true, token: "secret" });
    sessionStorage.setItem("te_access_token", "secret");
    renderApp("/config");

    await screen.findByRole("heading", { name: "Config" });
    expect(screen.queryByRole("heading", { name: "Access token required" })).toBeNull();
  });
});
