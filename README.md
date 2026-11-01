# Maru

**An Edge AI desk companion that senses, learns and logs to Obsidian**

*Maru (丸) means "circle" in Japanese. It's also how "0" is often read aloud in Japanese, and the everyday name for the ゜ mark. While building the first prototype, I found that the LCD's "degree symbol" is actually this Japanese mark from the display controller's katakana character table.*

Maru is a desk device that senses its environment, recognizes what is happening at the desk using a machine-learning model running directly on the microcontroller (Edge AI), and logs everything to Obsidian notes. A local language model on the PC turns the logs into daily summaries. Everything runs locally: no cloud services.

## Architecture

```
[Sensors] → [ESP32: reads sensors, runs Edge AI model] --Wi-Fi--> [PC: Python script + local LLM (Ollama)] → [Obsidian vault]
```

- **Device:** ESP32 with temperature/humidity, light, motion and sound sensors, a button and an LCD
- **On-device AI:** a small model trained on the collected sensor data classifies the desk state (e.g. studying / at desk / away)
- **PC:** receives the data, writes it into Obsidian daily notes, and uses a local language model to summarize it

## Roadmap

| Stage | Goal | Status |
|---|---|---|
| 1. Basics | Read all sensors and a button on an Arduino, show values on an LCD | ✅ Done |
| 2. First working version | Move to the ESP32 and send data over Wi-Fi to a Python script that writes into Obsidian | ⏳ Next |
| 3. Data collection | Record labeled sensor data for different desk states | ⬜ |
| 4. Edge AI | Train a model with Edge Impulse and run it on the ESP32 | ⬜ |
| 5. Local LLM | Use Ollama to generate daily summaries in Obsidian | ⬜ |
| 6. Documentation | Wiring diagram, photos, demo video | ⬜ |

**Planned later:** custom PCB, voice command recognition on the ESP32, better sensors.

## Stage 1: Arduino prototype

### Hardware
- Arduino board (Elegoo starter kit)
- DHT11 temperature and humidity sensor
- Photoresistor with 10 kΩ resistor (voltage divider)
- HC-SR501 PIR motion sensor
- Sound sensor module (analog output)
- Push button
- 16x2 LCD (HD44780) with 10 kΩ potentiometer for contrast

### Pin assignment

| Component | Pin |
|---|---|
| DHT11 signal | D2 |
| Button | D3 (internal pull-up) |
| PIR output | D4 |
| Photoresistor (divider midpoint) | A0 |
| Sound sensor AO | A1 |
| LCD RS, E, D4, D5, D6, D7 | D7, D8, D9, D10, D11, D12 |

### What it does
- Reads all sensors every 2 seconds without blocking, using `millis()` instead of `delay()`
- Toggles a study session with the button (edge detection and debouncing)
- Measures loudness as the peak-to-peak range of the microphone signal over each 2-second window
- Shows temperature, humidity and session state on the LCD
- Outputs all values in `label:value` format for the Arduino Serial Plotter

## What I learned

**The LCD speaks katakana.** When I typed `°` into the LCD code, the display showed ツー instead. My computer stores `°` as two bytes (UTF-8), and the LCD reads each byte as a separate character from its own table. Its HD44780 controller was made by Hitachi and has katakana in its character table. The "degree symbol" everyone uses, `(char)223`, is actually the Japanese handakuten ゜.

**`delay()` blocks everything.** After adding a button, presses were often ignored, because the board spent almost all its time inside `delay(2000)`. I replaced it with non-blocking timing using `millis()`: the loop now runs constantly, checks the button every time, and only reads the sensors when 2 seconds have passed.

**Reading a button properly takes three things.** A pin with nothing connected "floats" and reads random values, so I used the built-in pull-up resistor (`INPUT_PULLUP`), which means the pin reads LOW when pressed. Because the loop reads the button thousands of times per second, I added edge detection so it only reacts to the moment the button goes from released to pressed. And because the metal contacts bounce for a few milliseconds, I ignore any new press within 200 ms of the last one (debouncing).

## Repository structure

```
firmware/
  desk_assistant_v1/
    desk_assistant_v1.ino   # Stage 1 Arduino sketch
images/                     # Photos and wiring diagrams
README.md
```
