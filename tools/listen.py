import sys, time, serial
port = sys.argv[1] if len(sys.argv) > 1 else "COM4"
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 20.0
# Listen only. No DTR/RTS toggle, so the board is NOT reset: this observes
# steady-state idle traffic on an already-booted board.
s = serial.Serial(port, 115200, timeout=0.2)
s.reset_input_buffer()
t0 = time.time()
buf = b""
while time.time() - t0 < secs:
    d = s.read(4096)
    if d:
        buf += d
s.close()
txt = buf.decode("utf-8", "replace")
print("bytes captured: %d over %.1fs" % (len(buf), time.time() - t0))
print("i2c error lines: %d" % txt.count("i2c_master_transmit"))
print("total lines: %d" % len(txt.splitlines()))
if txt.strip():
    print("--- content ---")
    print(txt[:2000])
else:
    print("--- port silent for the whole window ---")
