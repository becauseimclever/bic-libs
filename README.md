# BIC Pico libraries

A collection of C libraries for the Raspberry Pi Pico SDK. Libraries live in
`src/<library>`, expose headers under `bic/`, and provide CMake interface
targets. Examples live in `examples/<library>`.

Future feature plans are in [docs](docs/README.md).

## Using the libraries

In a Pico SDK application, initialize the SDK and then include
`bic_libs_import.cmake` from this checkout:

```cmake
pico_sdk_init()
include(path/to/bic-libs/bic_libs_import.cmake)
target_link_libraries(my_app PRIVATE bic_ntp pico_cyw43_arch_lwip_poll)
```

The application selects and initializes its own network interface and lwIP
architecture. The NTP library only depends on lwIP, not Wi-Fi hardware.

## NES and SNES controllers

Link `bic_nes_snes` and include `bic/nes_snes.h` to read a standard NES or
SNES controller or emulate one for a console. The implementation is adapted
from [CHIRBot](https://github.com/Fortinbra/CHIRBot); its MIT license is
retained in [third_party/CHIRBot/LICENSE](third_party/CHIRBot/LICENSE).
Accessories, multitaps, playback queues, and other controller protocols are
not implemented.

Both roles use distinct, application-selected LATCH, CLOCK, and DATA GPIOs.
The controller reader drives LATCH and CLOCK and reads DATA; the emulator
receives LATCH and CLOCK and drives DATA. **NES/SNES interfaces use 5 V:
Pico GPIOs are not 5 V tolerant.** Use appropriately powered, direction-
appropriate 5 V/3.3 V level shifting on all three signals, a common ground,
and a suitable controller supply. Isolate DATA during reset, startup, and
power loss with a reset-safe hardware output-enable circuit; do not rely on
firmware for protection. Power down both sides before changing wiring. Verify
the connector pinout and electrical behavior for the actual console and
controller; no specific level shifter is qualified by this library.

Initialize a `bic_nes_snes_reader_t` with `BIC_NES_SNES_READER_INIT`, call
`bic_nes_snes_reader_init` with `BIC_NES_SNES_NES` or `BIC_NES_SNES_SNES`,
then call `bic_nes_snes_reader_read` to poll. It blocks for at least 114 us
for NES or 210 us for SNES: a 12 us LATCH pulse, followed by 8 or 16 samples
and 6 us CLOCK half-periods. Interruptions can lengthen the poll. CLOCK idles
high and LATCH low; pulls are disabled and any needed pull-ups must be
provided externally. The library does not detect disconnected controllers
or debounce input.

The returned `uint16_t` is an active-high serial-order button mask. NES bit
order is A, B, Select, Start, Up, Down, Left, Right. SNES order is B, Y,
Select, Start, Up, Down, Left, Right, A, X, L, R, then four normally released
reserved bits. Named `BIC_NES_*` and `BIC_SNES_*` masks are provided; the
reader preserves all 16 SNES bits. Keep SNES bits 12-15 clear when emulating a
standard controller.

For console emulation, initialize a `bic_nes_snes_emulator_t` with
`BIC_NES_SNES_EMULATOR_INIT`, call `bic_nes_snes_emulator_init` with the
profile, a PIO instance, and pins, and call
`bic_nes_snes_emulator_set_buttons` with the active-high mask. For example:

```c
bic_nes_snes_emulator_t controller = BIC_NES_SNES_EMULATOR_INIT;
bic_nes_snes_status_t status =
    bic_nes_snes_emulator_init(&controller, BIC_NES_SNES_NES, pio0, 2, 0, 4);
if (status == BIC_NES_SNES_OK) {
    status = bic_nes_snes_emulator_set_buttons(&controller, BIC_NES_A | BIC_NES_START);
}
if (status != BIC_NES_SNES_OK) {
    /* Handle the error; keep the external output disabled. */
}
```

The emulator claims one PIO state machine and 19 instruction slots; each
instance loads its own program. Pins must be accessible from the selected
PIO's current GPIO base; the library does not change that shared base.
Initialization can fail if resources are unavailable. It starts released,
drives active-low DATA, shifts after rising CLOCK, and repeats the last state
until changed. NES sends low after eight bits; SNES sends high after sixteen.
Updates during a latched report are applied on a later LATCH; while LATCH is
held high, the first bit can update. Only the latest pending state is kept,
not a queue, and the last state remains indefinitely if the application stops.
Serialize calls on each instance; do not share its pins or PIO state machine.
Call the corresponding `*_deinit` to release resources and return the pins to
inputs with pulls and GPIO overrides disabled; previous pin configuration is
not restored. Profile changes require deinitialization and reinitialization
with console polling stopped.

These are protocol implementations, not electrically qualified devices.
Validate voltage, rise times, reset/power sequencing, and waveforms with the
intended hardware and a logic analyzer before connecting a console.

## USB device protocols

The USB protocol libraries originated from and were inspired by
[GP2040-CE](https://github.com/OpenStickCommunity/GP2040-CE), particularly its
XInput implementation. The XInput configuration descriptor is adapted from
GP2040-CE; its MIT license and copyright notices are retained in
[third_party/GP2040-CE/LICENSE](third_party/GP2040-CE/LICENSE). Keyboard and
mouse report layouts also draw on TinyUSB; see
[third_party/TinyUSB/LICENSE](third_party/TinyUSB/LICENSE).

Link the profiles needed by the application: `bic_usb_xinput`,
`bic_usb_dinput`, `bic_usb_keyboard`, and/or `bic_usb_mouse`. Each includes
`bic_usb_device`. These libraries provide **descriptors and report
encoding/decoding only**: they do not configure USB, install TinyUSB callbacks,
read GPIOs, send reports, or implement a complete controller. The application
selects its profile at startup and owns USB initialization, endpoint handling,
transfer scheduling, and host requests. Only one USB device configuration can
be active at a time.

Each profile exposes `bic_usb_<profile>_profile`, containing its configuration
descriptor and, for HID profiles, its report descriptor. Call
`bic_usb_device_descriptor` with that profile and the application's VID/PID to
build the 18-byte device descriptor. The generated descriptor has no string
indices; an application adding strings must update those indices and implement
the corresponding USB callbacks. Descriptor lengths are available in the
profile; report sizes are declared in the headers. Use each profile's
`*_encode` function to produce a report, and check its status before
transferring it.

The DirectInput-compatible profile is a generic HID gamepad with six signed
8-bit axes (-127 to 127), 32 buttons, and an eight-way hat (0-7 clockwise,
8 neutral). The keyboard profile uses six-key rollover and modifier bits; its
LED output can be decoded with `bic_usb_keyboard_decode_leds`. The mouse
profile uses five buttons and relative X/Y, vertical wheel, and horizontal pan
in the range -127 to 127. Keyboard and mouse are separate non-boot HID
interfaces. A zeroed keyboard report releases all keys.

The XInput profile describes the Xbox 360 wired protocol and encodes its
20-byte gamepad report; `bic_usb_xinput_decode_output` handles rumble and LED
packets. It is **vendor-specific, not HID**. A TinyUSB application must provide
an appropriate custom class driver and endpoint/control handling; descriptors
alone do not make a Pico appear as an XInput device. Auxiliary audio and
security interfaces in the descriptor are not implemented here. Xbox console
authentication is outside this library's scope.

Windows XInput driver binding depends on the VID/PID and host setup. The
`BIC_USB_XINPUT_TEST_VID`/`PID` constants match Microsoft's Xbox 360 wired
identity as used by GP2040-CE, for development/testing only; they are not
identifiers for a distributed product. Use authorized IDs for products and
arrange appropriate Windows driver binding. Give distinct configurations
distinct product IDs to avoid host driver-cache conflicts. Host enumeration
and application compatibility must be verified with the completed USB
integration; protocol tests alone cannot establish them.

## ULN2003AN and 28BYJ-48 stepper

Link `bic_uln2003` and include `bic/uln2003.h`. This driver targets the
four-phase unipolar 28BYJ-48 motor and its common ULN2003 driver board; it
does not drive bipolar steppers or provide a general seven-output ULN2003 API.
Configure four GPIOs in the board's IN1, IN2, IN3, IN4 order and select
`BIC_ULN2003_HALF_STEP` or two-coil `BIC_ULN2003_FULL_STEP`. Positive and
negative steps traverse the sequence in opposite directions; physical rotation
depends on the motor and wiring.

Initialize a `bic_uln2003_t` with `BIC_ULN2003_INIT`. `bic_uln2003_init`
configures the GPIOs with all coils off. `bic_uln2003_move` queues a nonzero
signed step count, a step interval in microseconds, and whether to hold the
coils after the move. Call `bic_uln2003_update` frequently from the application;
it emits at most one due step per call and reports whether the move remains
active. The first update emits the first phase immediately. After the last
phase, the move stays active for one interval so that the motor has time to
respond before the coils are released or held. A late update does not catch up:
the move takes longer instead of producing a burst of steps.

`bic_uln2003_stop` cancels immediately and can hold or release the coils;
`bic_uln2003_deinit` stops and releases them. A held motor consumes power and
can heat up; use holding only within the motor and board ratings. If servicing
stops, the last coil pattern remains energized, even when release was requested
at the end of the move. The driver
does not provide acceleration, feedback, or a safety-rated holding mechanism.
The first energized phase may align the rotor rather than produce a full
physical step, and releasing coils can lose position. Gear ratios vary; do not
assume an exact number of steps per output revolution without calibration.
Choose an interval suitable for the motor and load. Reserve the GPIOs and
synchronize calls when using multiple cores or interrupts.

**Wiring:** Connect the four Pico GPIOs to the board inputs, use a motor supply
appropriate to the specific 28BYJ-48 and board, and connect the motor-supply
ground to Pico ground. Do not connect motor coils directly to Pico GPIOs. For a
bare ULN2003AN, connect its COM clamp-diode pin to the motor supply positive
and follow the [TI datasheet](https://www.ti.com/lit/ds/symlink/uln2003a.pdf)
for coil and power wiring; verify that a premade board has the corresponding
clamp connection. Observe current, voltage, and thermal limits. Keep the
mechanism clear and test unloaded.

### ULN2003 example

Wire IN1-IN4 to GP2-GP5, then build the USB-serial example:

```sh
cmake -S . -B build -DPICO_BOARD=pico -DBIC_LIBS_BUILD_EXAMPLES=ON \
  -DBIC_LIBS_BUILD_NTP_EXAMPLE=OFF -DBIC_LIBS_BUILD_DRV8833_EXAMPLE=OFF \
  -DBIC_LIBS_BUILD_A3144_EXAMPLE=OFF
cmake --build build
```

Flash `build/examples/uln2003/bic_uln2003_example.uf2`, open USB serial, and
send `?` for commands. `f` and `r` request 128 half-steps at a 5 ms interval;
`s` stops and releases. Nothing moves automatically. See
[stepper.c](examples/uln2003/stepper.c) for the application-serviced loop.

## A3144 Hall-effect switch

Link `bic_a3144` and include `bic/a3144.h`. The A3144 is a digital,
active-low magnetic switch, not an analog field sensor. Its built-in magnetic
hysteresis controls the operate and release thresholds. The reported state is
the electrical output level, not a measurement of field strength.

**Wiring:** The original
[Allegro A3144 datasheet](https://www.alldatasheet.com/datasheet-pdf/view/55092/ALLEGRO/A3144.html)
specifies a 4.5-24 V sensor supply and an open-collector output. Supply the
sensor according to its actual datasheet, connect its ground to Pico ground,
and pull its output up to **3.3 V**, not the sensor's higher supply. The Pico
GPIO input is not 5 V tolerant. Some breakout boards already pull the signal up
to their supply: inspect the schematic and remove or safely level-shift that
connection before connecting a 5 V-powered board to the Pico. The Pico's
internal pull-up cannot make an existing 5 V pull-up safe. Verify the actual
sensor or breakout pinout and ratings; compatible parts may differ.

Initialize a `bic_a3144_t` with `BIC_A3144_INIT`, then call `bic_a3144_init`
with a GPIO, whether to enable the Pico's weak internal pull-up, and an
optional callback and callback argument. Use `bic_a3144_read` to sample the
active-low output. Set the pull-up option to false when using a suitable
external 3.3 V pull-up. Reserve the GPIO for the sensor and disable any
previously configured IRQ for it before initialization. Check each function's
status.

For interrupts, the application owns GPIO IRQ registration and forwards this
pin's `GPIO_IRQ_EDGE_FALL` and `GPIO_IRQ_EDGE_RISE` event mask to
`bic_a3144_handle_irq` from its GPIO handler. The library then calls the
registered callback in the caller's context, normally an interrupt handler,
with `BIC_A3144_DETECTED`, `BIC_A3144_RELEASED`, or both. Keep callbacks short
and IRQ-safe. The library does not replace the SDK's shared GPIO callback, so
applications can dispatch multiple GPIOs themselves. The SDK's
`gpio_set_irq_enabled_with_callback` replaces the existing callback on that
core; integrate with the application's existing handler if one is already in
use. Disable the pin's IRQ before `bic_a3144_deinit`, keep the sensor object
alive until then, and synchronize lifecycle operations with IRQ dispatch.

Edge bits can be coalesced before servicing, so both flags do not imply an
ordering or exact transition count. Read the current state separately if
needed. There is no software debounce or pulse counter.

### A3144 example

After wiring the output safely to GP15, build the USB-serial example:

```sh
cmake -S . -B build -DPICO_BOARD=pico -DBIC_LIBS_BUILD_EXAMPLES=ON \
  -DBIC_LIBS_BUILD_NTP_EXAMPLE=OFF -DBIC_LIBS_BUILD_DRV8833_EXAMPLE=OFF
cmake --build build
```

Flash `build/examples/a3144/bic_a3144_example.uf2` and open USB serial. The
example uses the internal pull-up, samples the initial state, and reports
interrupt events. See [hall_sensor.c](examples/a3144/hall_sensor.c) for the
SDK GPIO callback and dispatch setup.

## DRV8833 dual motor driver

Link `bic_drv8833` for two brushed DC motors. Initialize a
`bic_drv8833_t` with `BIC_DRV8833_INIT` and configure the four DRV8833
inputs in AIN1, AIN2, BIN1, BIN2 order. The Pico Motor SHIM preset follows
[Pimoroni's published pinout](https://github.com/pimoroni/pimoroni-pico/blob/main/micropython/modules/motor/README.md#motor-pins):
motor 1 uses GP6/GP7 and motor 2 uses GP27/GP26. Its sleep and fault signals
are not exposed to the Pico by the preset.

```c
#include "bic/drv8833.h"

bic_drv8833_t motors = BIC_DRV8833_INIT;
bic_drv8833_config_t config = BIC_DRV8833_PICO_MOTOR_SHIM_CONFIG;
/* Or: bic_drv8833_config_t config = { {{2, 3}, {4, 5}},
 *     BIC_DRV8833_NO_PIN, BIC_DRV8833_NO_PIN, 20000 }; */
if (bic_drv8833_init(&motors, &config) != BIC_DRV8833_OK) {
    /* Handle the configuration error. */
}
```

`bic_drv8833_drive` accepts signed duty from -10000 to 10000; zero coasts.
Positive means IN1 PWM/IN2 low and negative reverses the inputs. The physical
direction depends on motor wiring. `bic_drv8833_coast` releases the bridge;
`bic_drv8833_brake` shorts the motor outputs for braking. Drive PWM uses fast
decay. Check the returned status from each operation.

Both bridges start coasting. Optionally connect `nSLEEP` and `nFAULT` to GPIOs,
or set either to `BIC_DRV8833_NO_PIN` when not connected. If sleep is not
connected, the board must hold `nSLEEP` high independently. Sleep clears both
commands; wake waits 1 ms and leaves both motors coasting. Fault is active-low
and open-drain; ensure a suitable pull-up to 3.3 V (the driver also enables the
Pico's weak internal pull-up). The fault API reports the pin level; it does not
automatically stop or restart motors. Deinitialization coasts both bridges and
asserts sleep when available.

Initialization reserves the configured GPIOs and **entire PWM slices**: do not
share those slices with other PWM users or other driver instances. Two GPIOs
that map to the same PWM slice and channel cannot be used together. The
frequency is set for all four inputs, may be rounded to hardware resolution,
and must be representable at the current system clock. Do not change that clock
while the driver is initialized. Calls require external synchronization if
used across cores or interrupts.

Use motor power appropriate to the particular board, with a common ground when
using separate supplies, and stay within the board's voltage, current, and
thermal limits. Never connect motors directly to Pico GPIOs. The SHIM's current
limit is set by its hardware. Test with mechanisms unloaded and clear before
driving; braking is not a safety-rated holding mechanism. Coast and allow the
motor to slow before reversing when the mechanism requires it.

### Motor SHIM example

To build the USB-serial example without Wi-Fi or credentials:

```sh
cmake -S . -B build -DPICO_BOARD=pico -DBIC_LIBS_BUILD_EXAMPLES=ON \
  -DBIC_LIBS_BUILD_NTP_EXAMPLE=OFF
cmake --build build
```

Flash `build/examples/drv8833/bic_drv8833_example.uf2`, open USB serial, and
send `?` for commands. Motors remain stopped until commanded. The example uses
25% duty; `s` coasts both and `b` brakes both. Commands persist until changed
or reset. For a different breakout, replace the preset with the wiring above.

## NTP client

`bic_ntp` sends one asynchronous NTPv4 client request to a resolved IPv4 or
IPv6 address. Initialize a `bic_ntp_client_t` with `BIC_NTP_CLIENT_INIT`, then
call `bic_ntp_request`. The callback receives either a server transmit time or
a timeout; the time pointer is only valid during the callback. A successful
start returns `ERR_OK` and invokes the callback once. An immediate lwIP error
is returned without invoking it. Call `bic_ntp_cancel` to stop a request
without a callback, and keep the client alive until completion or cancellation.
Only one request per client may be active; a completed client can be reused,
including from its callback.

Call the API in the lwIP core context or while holding the port's lwIP lock.
For example, use `cyw43_arch_lwip_begin()` and `cyw43_arch_lwip_end()` from
application code using Pico W; lwIP callbacks already run in the correct
context. Continue servicing the network and lwIP timers as required by the
chosen architecture. Resolve hostnames separately using lwIP DNS; see the
example.

The result contains 64-bit Unix seconds and the original 32-bit binary
fraction (fraction / 2^32 seconds). Era unfolding assumes dates from
1970-01-01 through 2106-02-07. It reports the server transmit timestamp, not
round-trip compensated time, and does not set a clock. This is an unauthenticated
single-server query, not a full NTP clock discipline or a security source of
time. Invalid or unrelated packets are ignored until the request times out.

## Pico W example

The Raspberry Pi Pico VS Code extension can supply the SDK and build tools.
Set `PICO_SDK_PATH` to its installed SDK (or pass it as a CMake option), and
select a Pico W-compatible board. Alternatively, use an initialized SDK
checkout with its lwIP and cyw43-driver submodules:

```sh
cmake -S . -B build -DPICO_BOARD=pico_w -DBIC_LIBS_BUILD_EXAMPLES=ON \
  -DWIFI_SSID=your_ssid -DWIFI_PASSWORD=your_password
cmake --build build
```

Flash `build/examples/ntp/bic_ntp_example.uf2` and open USB serial output. The
example connects with WPA2, resolves `pool.ntp.org`, and prints one result.
Credentials passed to CMake are stored in the build directory and embedded in
the firmware; keep both private. Building the libraries without examples does
not require credentials.

## Host tests

The host tests use small lwIP and hardware test doubles and do not require a Pico SDK:

```sh
cmake -S tests -B build-tests
cmake --build build-tests
ctest --test-dir build-tests --output-on-failure
```

## License

This project is licensed under the [GNU Affero General Public License v3.0](LICENSE)
(AGPL-3.0-only). Third-party material retains its original MIT licenses; see
the notices in [third_party](third_party).
