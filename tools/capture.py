import sys, time, serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM4"
secs = float(sys.argv[2]) if len(sys.argv) > 2 else 25.0

s = serial.Serial(port, 115200, timeout=0.2)
# ESP32-C3 USB Serial/JTAG: RTS is wired to EN. Pulse it to hard-reset.
s.setDTR(False)
s.setRTS(True)
time.sleep(0.2)
s.setRTS(False)
s.reset_input_buffer()

t0 = time.time()
buf = b""
while time.time() - t0 < secs:
    d = s.read(4096)
    if d:
        buf += d
        sys.stdout.buffer.write(d)
        sys.stdout.flush()
s.close()
sys.stderr.write("\n--- captured %d bytes in %.1fs ---\n" % (len(buf), time.time() - t0))
