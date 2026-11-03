# 実機の画面をシリアルで受け取って PNG にする (確かめる用)。
#   python tools/snap.py COM14 出力の頭 [コマンド...]
# コマンド: s = 1 枚写す、f = 10 秒先へ進める、n = SW1 を押す、m = 母船の直前へ、w<秒> = 待つ、log = 速さの記録を表示
# 例: python tools/snap.py COM14 shots/a s w3 s f f s
import sys
import time
import serial
from PIL import Image


def read_until(ser, token, timeout=10):
    buf = b""
    end = time.time() + timeout
    while time.time() < end:
        buf += ser.read(ser.in_waiting or 1)
        i = buf.find(token)
        if i >= 0:
            return buf[:i], buf[i + len(token):]
    raise TimeoutError(token)


def snap(ser, path):
    ser.reset_input_buffer()
    ser.write(b"s")
    log, rest = read_until(ser, b"FRAME 240 320\n")
    need = 240 * 320 * 2
    data = rest
    end = time.time() + 20
    while len(data) < need and time.time() < end:
        data += ser.read(need - len(data))
    if len(data) < need:
        raise TimeoutError("frame data")
    img = Image.new("RGB", (240, 320))
    px = img.load()
    for i in range(240 * 320):
        v = data[i * 2] | (data[i * 2 + 1] << 8)
        r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
        px[i % 240, i // 240] = (r << 3 | r >> 2, g << 2 | g >> 4, b << 3 | b >> 2)
    img = img.resize((480, 640), Image.NEAREST)
    img.save(path)
    print("saved", path)


def main():
    port, prefix, cmds = sys.argv[1], sys.argv[2], sys.argv[3:]
    ser = serial.Serial()  # 開くときに DTR / RTS を動かさない (動かすと基板がリセットされる)
    ser.port, ser.baudrate, ser.timeout = port, 921600, 0.5
    ser.dtr = ser.rts = False
    ser.open()
    n = 0
    for c in cmds:
        if c == "s":
            snap(ser, "%s_%02d.png" % (prefix, n))
            n += 1
        elif c in ("f", "n", "m"):
            ser.write(c.encode())
            time.sleep(0.3 if c == "n" else 1.5)
        elif c.startswith("w"):
            time.sleep(float(c[1:]))
        elif c == "log":
            end = time.time() + 6
            while time.time() < end:
                line = ser.readline().decode("ascii", errors="ignore").strip()
                if line:
                    print(line)
    ser.close()


main()
