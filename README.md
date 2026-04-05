# DesktopWatcher

> Note: Work in progress — tested on Ubuntu (Wayland/GNOME)

A lightweight background daemon that passively tracks active window usage on Linux, logs session data to a local SQLite database, and provides a CLI for reporting and insights. Self-hosted, privacy-first — no data leaves the machine.

## Components

| Binary             | Alias | Description                                                   |
| ------------------ | ----- | ------------------------------------------------------------- |
| `desktop-watcherd` | `dwd` | Daemon process — polls the active window and records sessions |
| `desktop-watcher`  | `dw`  | CLI client — queries the database and renders reports         |

## Dependencies

### System packages (apt)

```
sudo apt-get install \
    libfmt-dev \
    libsqlite3-dev \
    libtomlplusplus-dev \
    libcli11-dev \
    libxss-dev \
    libx11-dev \
    libatspi2.0-dev \
    libglib2.0-dev \
    ninja-build \
    cmake
```

### Notes

- **Xlib / XScreenSaver** (`libx11-dev`, `libxss-dev`) — X11 window detection and idle time.
- **AT-SPI2** (`libatspi2.0-dev`) — active window detection on Wayland (GNOME).
- **GLib / GIO** (`libglib2.0-dev`) — D-Bus calls to Mutter for idle time on Wayland.
- **toml++** (`libtomlplusplus-dev`) — config file parsing.
- **fmt** (`libfmt-dev`) — string formatting.
- **CLI11** (`libcli11-dev`) — argument parsing for the CLI.
- **SQLite3** (`libsqlite3-dev`) — local session database.

## Building

```
cmake --preset default      # configure (Debug build in ./build/)
cmake --build build         # compile all targets
```

For a release build:

```
cmake --preset release
cmake --build build
```

### Running tests

```
cd build && ctest --output-on-failure
```

## Configuration

Copy the example config to the XDG config directory:

```
mkdir -p ~/.config/desktop-watcher
cp config/config.example.toml ~/.config/desktop-watcher/config.toml
```

The database is stored at `~/.local/share/desktop-watcher/desktop-watcher.db` by default. Both paths can be overridden in `config.toml`.

## Running

```
./build/dwd       # run daemon in foreground (Ctrl-C to stop)
./build/dw status # check what's currently being tracked
```

For background operation, install the systemd unit:

```
mkdir -p ~/.local/bin
cp build/desktop-watcherd ~/.local/bin/
cp build/desktop-watcher  ~/.local/bin/
systemctl --user enable --now "$(pwd)/config/desktop-watcher.service"
```
