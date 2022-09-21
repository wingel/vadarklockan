Installera lite saker som behövs

```
pip3 install adafruit-ampy esptool python-is-python3
```

Hämta micropython med stöd för ed25519 och sha512

```
git clone https://github.com/wingel/MicroPython_ESP32_psRAM_LoBo
```

Flasha ESP32 med MicroPython

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

Kopiera "rt_config.py.example" till "rt_config.py" och ändra den filen
så att den har rätt WLAN_SSID och WLAN_PASSWORD, samt ladda upp den
och "ecosystem.json" till filsystemet på ESP32:

```
ampy --port /dev/ttyACM0 --baud 115200 put rt_config.py /flash/rt_config.py
ampy --port /dev/ttyACM0 --baud 115200 put ecosystem.json /flash/ecosystem.json
```

Korskompilera "rt.py" till "rt.mpy" och ladda upp den till filsystemet
på ESP32:

```
MicroPython_BUILD/components/mpy_cross_build/mpy-cross/mpy-cross rt.py
ampy --port /dev/ttyACM0 --baud 115200 put rt.mpy /flash/rt.mpy
```

Nu kan man köra "rt_main.py" via thonny.

Alternativt så kan man använda "run.py" för att både kors-kompilera
och ladda upp filer till ESP32.  Modifiera konfigurationen i början på
run.py så att sökvägarna till /dev/ttyACM0 och mpy-cross stämmer.

```
python3 run.py
```
