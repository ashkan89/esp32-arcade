# Optional controls and audio

These additions are inferred from **repository pin usage**. Carrier-board circuitry or the
AH21 device may consume additional pins; verify the carrier schematic before adding wires.
None are required for the default console.

## Expanded controls

| Function | GPIO | Wiring / qualification |
| --- | ---: | --- |
| Left | 32 | Existing button to GND, internal pull-up |
| Right | 33 | Existing button to GND, internal pull-up |
| Fire / Confirm | 25 | Existing button to GND, internal pull-up |
| Special / Back | 14 | Existing weapon button to GND, internal pull-up |
| Up | 21 | Optional button to GND, internal pull-up; no repository assignment |
| Down | 22 | Optional button to GND, internal pull-up; no repository assignment |
| Pause / Menu | 34 | Optional button to GND; **external 10 kΩ to 3.3 V**, input-only, no internal pull-up |

Use `pio run -e expanded` and upload that environment only with the GPIO34 pull-up fitted.
Missing pull-up can make the input float and cause spurious pauses. GPIO21/22 remain available
for I²C only in the baseline; expanded mode uses them as buttons. The original SD bus,
GPIO26 sensor label, and display pins are retained. GPIO6–11 serve flash; GPIO16/17 serve
WROVER PSRAM. Avoid new loads on strapping pins 0/2/5/12/15. GPIO35/36/39 are also input-only
and lack internal pull-ups; UART GPIO1/3 are not proposed for controls.

## GPIO27 buzzer

The project already has GPIO27 tone code. The transducer's actual part, impedance, and drive
circuit are not recorded. A **passive** piezo or magnetic buzzer follows PWM frequency;
an **active** buzzer contains its own oscillator and generally only beeps at its fixed pitch.
Neither is a PCM speaker/DAC. The firmware offers mute and compile-time audio disable,
with fixed 50% duty for sound; it deliberately offers no invented arbitrary volume scale.

For a small piezo, follow the component's rated voltage/current and the module's GPIO limits.
A 100–330 Ω series resistor may be appropriate, but is not a substitute for verifying the
component. Use a transistor driver for a magnetic buzzer or a load needing more current
than safe GPIO drive. Do not connect a 5 V transducer supply to a GPIO.

Example low-side driver (verify transducer current and transistor rating):

| Connection | Component / destination |
| --- | --- |
| GPIO27 | 1 kΩ resistor to NPN base (2N2222 or BC337) |
| Base | 100 kΩ resistor to GND |
| Emitter | GND |
| Collector | Buzzer negative |
| Buzzer positive | Rated 3.3 V supply, or rated 5 V supply with common GND |
| Magnetic coil protection | 1N4148 for suitable small coil current, or appropriately rated diode, across coil: cathode to supply, anode to collector |
| Supply decoupling | 100 nF near the transducer/driver; add bulk capacitance as its current requires |

Piezo elements do not require the magnetic-coil flyback diode. A higher-current coil may need
a MOSFET or different base resistor; choose from its actual electrical specification. The
base resistor limits GPIO current, while the transistor carries load current. GPIO27 is
neither a boot strap nor a WROVER PSRAM pin in the repository mapping.

Audio uses Arduino-ESP32 **2.0.17** channel-based LEDC APIs, channel 0, 8-bit resolution.
The library does not configure a TFT backlight PWM channel, so there is no application LEDC
timer/channel conflict. Every effect is a finite sequence, higher priorities preempt lower
ones, and `stop()` clears the current sequence. Menu music yields to effects. The `no_psram`
environment also compiles audio out with `ARCADE_AUDIO=0`; any environment can set that flag.
No GPIO27 waveform is configured when audio is disabled.

## Xbox option

`pio run -e xbox` enables BLE only for this build. It uses exact compatible Xbox/NimBLE
versions in `platformio.ini`, discovers the controller without a hard-coded personal MAC,
and accepts GPIO controls at the same time. Put the controller in pairing mode while in the
launcher. Discovery and reconnect may briefly stall a menu; reconnection during gameplay is
deferred until you pause or exit. Fresh HID reports publish a whole input mask atomically;
reports older than 500 ms release all controller controls. Verify your controller sends
periodic reports while a button is held. D-pad, A/B/X and Start/Xbox are supported; analog
sticks and a battery percentage are not shown. Only menu confirmation triggers brief rumble.

Bluetooth pairing, report layout, disconnect/reconnect and rumble are **compiled, not tested
on a physical controller**. GPIO-only firmware does not depend on BLE libraries.
