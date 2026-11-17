import { useState } from "preact/hooks";
import { sendQuietly } from "../api/client";
import { useResettableState } from "../useResettableState";

const RGB_ORDERS = ["RGB", "RBG", "GRB", "GBR", "BRG", "BGR"];

export const releaseLed = () => sendQuietly("/setup/led", { method: "POST" });

// `looks` holds the color seen for each WS2812 wire byte; together they spell the RGB order.
export function useLedMapping(savedOrder: string) {
  const [looks, setLooks] = useResettableState(savedOrder, (order) => Array.from(order));
  const [remapOpen, setRemapOpen] = useState(false);
  const order = looks.join("");
  const valid = RGB_ORDERS.includes(order);

  return {
    looks,
    setLooks,
    order,
    valid,
    // Color previews use the mapping being edited once it names each color once.
    previewOrder: valid ? order : savedOrder,
    remapOpen,
    setRemapOpen,
  };
}

export type LedMapping = ReturnType<typeof useLedMapping>;
