# OpenNetview

**An open-source firmware replacement and desktop client for the Netview NI-3421 IP camera.**

OpenNetview aims to replace the proprietary firmware and software of the Netview NI-3421 IP camera with an open-source alternative. The project consists of custom camera firmware and a cross-platform desktop application for viewing and interacting with the camera.

The goal is to provide a locally controlled IP camera without relying on proprietary applications, cloud services, or vendor infrastructure.

## Features

### Camera Firmware
- Open-source replacement for the original Netview camera firmware.
- H.264 video encoding and RTP streaming over UDP.
- TCP connection for remote commands and camera control.
- Bidirectional communication between the camera and desktop client.
- Built using the OpenIMP interface and native camera hardware capabilities.

### Desktop Client
- Cross-platform desktop application built with C++ and SDL2.
- Live H.264 video playback.
- Video and application interface rendered in the same window.
- On-screen text overlays and camera information.
- TCP communication for sending commands to the camera.
- UDP-based reception of the camera's video stream.

## Architecture

OpenNetview separates video streaming from camera control using two network connections.

```text
             OpenNetview Client
           ┌──────────────────────┐
           │                      │
           │     SDL2 Interface   │
           │                      │
           │  Video Rendering     │
           │  Text Overlays       │
           │  User Input          │
           │                      │
           └──────────┬───────────┘
                      │
             ┌────────┴────────┐
             │                 │
          TCP Socket        UDP Socket
        (Commands)       (H.264 / RTP)
             │                 ▲
             │                 │
             ▼                 │
           ┌──────────────────────┐
           │   Netview NI-3421    │
           │                      │
           │  OpenNetview         │
           │  Firmware            │
           │                      │
           │  Command Processing  │
           │  Video Encoding      │
           │  RTP Streaming       │
           │                      │
           └──────────────────────┘
```

### Network Communication

The camera and client communicate over two independent connections:

| Connection | Protocol | Purpose |
|---|---|---|
| Command channel | TCP | Camera commands and control |
| Video channel | RTP over UDP | H.264 video streaming |

When a client establishes a TCP connection, the camera begins streaming video to the client's IP address over UDP.

This separation allows camera control and video transmission to operate independently.

The video stream uses H.264 encoding with RTP packetization (payload type 96).

## Repository Structure

```text
OpenNetview/
├── client/       # Desktop client application
├── firmware/    # Camera firmware
├── inc/         # Shared headers and definitions
├── .gitmodules  # Git submodule definitions
└── LICENSE      # GPL-2.0 license
```

## Getting Started

### Prerequisites

The project uses different toolchains for the desktop application and camera firmware.

**Desktop Client**
- C++ compiler with C++11 support or newer
- SDL2
- FFmpeg libraries for H.264/RTP handling
- Git

**Camera Firmware**
- MIPS32 cross-compilation toolchain
- Camera-specific Linux development environment
- OpenIMP
- uvgRTP

The firmware targets the hardware and software environment of the Netview NI-3421 camera. Building it requires an appropriate cross-compilation environment.

### Clone the Repository

Clone the repository and initialize its submodules:

```bash
git clone --recurse-submodules https://github.com/SkibbleBip/OpenNetview.git
cd OpenNetview
```

If you have already cloned the repository without its submodules:

```bash
git submodule update --init --recursive
```

### Building

The client and firmware have separate build environments. Refer to the respective project directories for their build configurations and dependencies.

### Installing the Firmware

Installing OpenNetview requires replacing the camera's existing firmware executable with the newly compiled `main` binary.

**Warning:** Replacing the original firmware can render the camera unusable if the replacement binary is incompatible or the installation is interrupted. Always back up the original binary before proceeding.

1. Connect to the camera using SSH or another method that provides shell access.
2. Navigate to the firmware binary directory:

   ```bash
   cd /mnt/mtd/netvue/firmware/bin
   ```

3. Back up the original firmware executable:

   ```bash
   cp main main.original
   ```

4. Transfer the newly compiled OpenNetview `main` binary to the camera.
5. Replace the existing firmware executable with the newly compiled binary. For example, if the new binary was transferred to `/tmp/main`:

   ```bash
   cp /tmp/main /mnt/mtd/netvue/firmware/bin/main
   chmod +x /mnt/mtd/netvue/firmware/bin/main
   ```

6. Restart the camera to launch the new firmware.

**Restoring the Original Firmware**

If the replacement firmware does not work as expected, restore the original executable:

```bash
cp /mnt/mtd/netvue/firmware/bin/main.original \
   /mnt/mtd/netvue/firmware/bin/main
chmod +x /mnt/mtd/netvue/firmware/bin/main
```

Restart the camera to return to the original firmware.

## Development

OpenNetview is an ongoing reverse-engineering and development project.

Areas of development include:

- Expanding camera functionality and remote control capabilities.
- Improving video streaming reliability.
- Improving desktop client usability and cross-platform compatibility.
- Documenting camera hardware interfaces and firmware functionality.
- Reducing dependencies on proprietary camera software.

Contributions, bug reports, and suggestions are welcome.

## Compatibility

The initial target device is the **Netview NI-3421 IP camera**.

Compatibility with other Netview models or camera hardware has not been established.

## License

This project is licensed under the **GNU General Public License v2.0 (GPL-2.0)**. See [LICENSE](LICENSE) for the full license text.

## Disclaimer

OpenNetview is an independent open-source project and is not affiliated with or endorsed by Netview or the original camera manufacturer.

Use modified firmware at your own risk. Always retain a backup of the original firmware before attempting to replace it.
