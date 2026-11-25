# Bluetooth input and output profiles

## Goal

Plan reusable Bluetooth controller and HID building blocks for Pico SDK
applications. **Input** means receiving reports from a Bluetooth device;
**output** means presenting a Bluetooth device to a host. These roles have
different discovery, connection, and compatibility requirements.

Like the existing USB protocol libraries, proposed profile components should
focus on profile-specific metadata and report encoding/decoding rather than
being a complete controller firmware. The application owns the Bluetooth
stack lifecycle, connection management, pairing policy, storage, and mapping
between devices. Stack-specific integration helpers may be considered
separately if needed; they are not implemented or designed yet.

## Candidate profiles

| Transport and role | Initial candidates | Questions to resolve |
| --- | --- | --- |
| Bluetooth Low Energy HID device | Gamepad, keyboard, mouse | HID over GATT services, report maps, security, and host compatibility |
| Bluetooth Low Energy HID host | Receive supported gamepad, keyboard, and mouse reports | Discovery, report-map parsing, device variations, and notifications |
| Bluetooth Classic HID device | Gamepad, keyboard, mouse | Stack support, descriptors, security, and host compatibility |
| Bluetooth Classic HID host | Receive supported HID device reports | Stack support, discovery, pairing, and device-specific behavior |

All entries are planned candidates. BLE HID and Bluetooth Classic HID are
different transports and require separate compatibility and security
validation. Vendor-specific controller protocols, console authentication,
and features such as rumble are separate investigations, not implied by HID
support. No particular host, console, controller, or Bluetooth transport is
currently claimed as supported.

## Design direction

- Define report formats and state independently from radio initialization;
  include input, output, and feature reports only as each profile requires.
- Investigate whether existing USB HID report layouts or codecs can be reused
  without assuming USB descriptors or transport behavior apply unchanged.
- Define how host-side code handles report IDs, descriptors, malformed reports,
  disconnects, and unsupported devices.
- Specify ownership of pairing, bonding, key storage, encryption, user consent,
  reconnection, and release of held inputs after a disconnect. Avoid embedding
  credentials or silently accepting insecure connections.
- Verify radio availability, supported Bluetooth roles, Pico SDK/BTstack
  integration, dependencies, and license requirements for each board and
  selected stack before promising support. Non-wireless Pico boards may need
  separate hardware and integration.
- Keep mappings and bridges between Bluetooth, USB, and retro-console drivers
  in application code unless a reusable abstraction is justified.

## Suggested milestones

1. Choose a transport, role, board, and target host or controller; research
   the applicable specification, stack capabilities, and licensing.
2. Define report shapes and application/stack boundaries. Create host-side
   codec tests covering malformed and boundary inputs.
3. Provide a small integration example after the protocol components and stack
   responsibilities are clear. Exercise pairing, encryption, reconnects,
   disconnects, and report delivery on real hardware.
4. Document a tested compatibility matrix and limitations before claiming
   support. Add other roles and transports only after their requirements are
   independently verified.
