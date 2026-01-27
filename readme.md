# Solar Water Heater

Prototype PV-to-heat system using ESP32 and RS-485.

## Status
Rev 0.9 boards ordered. Parts in transit.
BOM in sheets contains changes that need to be made to footprints.
Enclosure and heatsink undecided

## High-Level Architecture
- PV positive bus shared
- Low-side switching per heater channel
- RS-485 daisy-chain over CAT5
- ESP32-S3 master (indoor) with display and buttons
- ESP32-S3 remote comm board that could standalone.  One for each PV+ bus.
- dumb heater nodes up to 8 per remote comm board.
- aux outputs, relays, temp sensors everywhere
- diversion via AC heating elements via onboard relays