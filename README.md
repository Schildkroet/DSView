![DreamSourceLab Logo](DSView/icons/dsl_logo.svg)

# DSView

DSView is a GUI program for supporting various instruments from [DreamSourceLab](http://www.dreamsourcelab.com), including logic analyzers, oscilloscopes, etc. DSView is based on the [sigrok project](https://sigrok.org).

The sigrok project aims at creating a portable, cross-platform, Free/Libre/Open-Source signal analysis software suite that supports various device types (such as logic analyzers, oscilloscopes, multimeters, and more).

# Changes in this fork

This fork tracks upstream DSView and adds the following.

## Oscilloscope

- **Histogram** — value and period histograms with jitter statistics, from the captured waveform.
- **Channel-to-channel measurement** — phase, delay and skew between two DSO channels.
- **Reference waveforms** — freeze a channel's waveform and overlay it on the live view for comparison.
- **More math operations** — alongside add/subtract/multiply/divide: running integral, derivative, absolute value, square, signed square root, and moving-average low-pass and high-pass with a configurable window.
- **Split channels** — give each DSO channel its own row instead of sharing one grid.
- **Y-scale reset**, and a wider calibration range.
- A **demo oscilloscope device**, so the DSO interface can be used without hardware.

## Display and interface

- **Trace and font scaling** — configurable trace font size, plus vertical zoom with `Ctrl+Shift+wheel`. Both persist across sessions.
- **Logic trace appearance** — adjustable signal line width and optional per-channel divider lines.
- **Catppuccin Latte and Frappé themes**, in addition to the existing light and dark ones.
- **German translation.**
- Smooth scrolling on high-resolution mice and touchpads.
- Tabbed right-hand docks, reworked titlebar, clearer protocol-decoder colours, and an option to skip the save prompt on exit.

## Platform and packaging

- **Ported to Qt6** (Qt5 still builds).
- **Wayland** — fixed window dragging.
- **Continuous builds** for Linux (`.deb` and AppImage) and Windows (portable zip and an Inno Setup installer). The installer also stages the WinUSB driver binding, so a fresh Windows system detects the hardware without manual driver setup; the `.deb` reloads udev rules on install for the same reason.

## Protocol decoders

- **MCP230XX** — Microchip 8/16-bit I²C I/O expanders.
- **TMP112** — Texas Instruments I²C temperature sensor.

# Status

The DSView software is in a usable state and has official tarball releases. However, it is still a work in progress. Some basic functionality is available and working, but other things are always on the TODO list.

# Download

Pre-built binaries are available on the [releases page](https://github.com/Schildkroet/DSView/releases).

# Useful links

- [dreamsourcelab.com](https://www.dreamsourcelab.com)
- [kickstarter.com](https://www.kickstarter.com/projects/dreamsourcelab/dslogic-multifunction-instruments-for-everyone)
- [sigrok.org](https://sigrok.org)

# Copyright and license

DSView software is licensed under the terms of the GNU General Public License
(GPL), version 3 or later.

While some individual source code files are licensed under the GPLv2+, and
some files are licensed under the GPLv3+, this doesn't change the fact that
the program as a whole is licensed under the terms of the GPLv3+ (e.g. also
due to the fact that it links against GPLv3+ libraries).

Please see the individual source files for the full list of copyright holders.
