Installera lite saker som behövs

```
pip3 install adafruit-ampy esptool
```

Hämta micropython med stöd för ed25519 och sha512

```
git clone https://github.com/wingel/MicroPython_ESP32_psRAM_LoBo
```

```
esptool.py -p /dev/ttyACM0 -b 921600 erase_flash
cd MicroPython_BUILD/firmware/esp32_psram_all_ed25519_sha512
../flash.sh -p /dev/ttyACM0 -b 921600
```

Bygg mpy_cross

```
cd MicroPython_BUILD/components/mpy_cross_build
make
```

Korskompilera och ladda upp en fil till filsystemet på ESP32:

```
MicroPython_BUILD/components/mpy_cross_build/mpy-cross/mpy-cross rt.py
ampy --port /dev/ttyACM0 --baud 115200 put rt.mpy /flash/rt.mpy
```

Sen kan man köra "rtm.py" via thonny.
