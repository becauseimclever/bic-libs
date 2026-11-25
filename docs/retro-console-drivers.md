# Retro-console input and output drivers

## Goal

Extend the Pico SDK libraries to read physical controllers and, where
practical, emulate controllers for retro consoles. Keep each protocol family
independently linkable and usable without a complete adapter application.
Applications remain responsible for mapping inputs, deciding which devices
to connect, and coordinating between drivers.

An **input** driver reads a physical controller; an **output** driver presents
a controller to a console. Each role is a separate capability: support for
one does not imply support for the other.

## Starting point and candidates

| Protocol family | Potential scope | Status |
| --- | --- | --- |
| NES and SNES | Standard controllers, input and output | Implemented; physical hardware validation remains outstanding |
| Atari-style joysticks | Standard digital joystick and buttons | Candidate |
| Sega Master System and Mega Drive/Genesis | Standard controllers; distinguish controller variants | Candidate |
| Nintendo 64 and GameCube | Standard controllers and supported commands | Candidate |
| PlayStation and PlayStation 2 | Standard digital and analog controllers | Candidate |

These are candidates rather than a fixed priority order. Confirm console
regions and revisions, connector variants, controller modes, and accessories
for each target before specifying support. Multitaps, light guns, memory
devices, rumble, and other accessories require separate investigation; do not
assume they work with the basic controller driver.

## Design direction

- Document button ordering, analog values, wire formats, timing, idle states,
  and error behavior for each supported controller and role.
- Expose explicit initialization, read or update, and cleanup operations,
  including resource ownership and concurrency requirements.
- Use GPIO, PIO, or other Pico peripherals as appropriate to the protocol.
  Specify whether operations block, the available update semantics, and how
  state behaves when the application stops updating it.
- Keep controller state separate from USB and Bluetooth report formats.
  Applications can translate between them; introduce shared abstractions only
  where multiple implementations demonstrate a need.
- Treat electrical interfaces as part of the design. Document supply and
  signal voltages, direction, level shifting, common ground, isolation,
  reset/power sequencing, and any board-specific restrictions. Never assume a
  console signal is safe to connect directly to a Pico GPIO.

## Suggested milestones

1. Bench-validate the existing NES/SNES reader and emulator with real
   controllers and consoles, including timing and electrical behavior.
2. Select a controller family and define its exact input and output scope,
   sources, wiring, and expected behavior before implementation.
3. Implement and test each role independently, including failure and resource
   cleanup paths. Add application examples only where useful.
4. Validate with representative controllers and console revisions; record
   known limitations and unsupported accessories.

Host-side tests should cover bit order, timing sequences, state changes,
interrupted polls, and error handling as relevant. PIO or peripheral models
do not establish electrical compatibility: verify real waveforms and voltage
levels with appropriate test equipment before claiming hardware support.
