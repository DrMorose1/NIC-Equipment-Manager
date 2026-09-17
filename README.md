# NIC Equipment Manager

A Windows desktop utility for managing IPv4 adapter settings, saved network profiles, device access, and common network diagnostics in one workflow.

## Overview

NIC Equipment Manager was built to simplify a set of Windows networking tasks that are normally spread across multiple settings pages, command-line tools, and repeated manual steps.

Experienced technicians can usually navigate those tools quickly from memory. The goal here was not to replace that knowledge, but to reduce friction by bringing the most common actions into one clear interface. That also makes the workflow more approachable for someone newer to IT who understands the networking concepts but has not yet developed the shortcuts and command-line muscle memory that come from repeating the same tasks over time.

## Features

- View and select Windows network adapters
- Load the current IPv4 configuration
- Switch between DHCP and static IPv4 configuration
- Save and reuse network profiles
- Apply primary and additional IPv4 networks
- Store common device targets
- Ping saved or free-form IPv4 targets
- Open device Web GUIs
- Run common diagnostics from an Advanced Tools workspace
- Launch elevated PowerShell or Command Prompt sessions
- Use built-in shortcuts for `ipconfig /all`, `route print`, `arp -a`, `netstat`, `tracert`, `pathping`, and `nslookup`
- Run free-form administrative commands and review, copy, clear, or save output
- Responsive Windows desktop layout
- Dark/system theme support

## Why I Built It

Windows already provides everything needed to configure adapters and troubleshoot network connectivity. The challenge is that the workflow can be fragmented and repetitive.

For an experienced user, that often means jumping between Settings, Control Panel, PowerShell, Command Prompt, and memorized commands. For someone newer to IT, the same workflow can be harder simply because they have not yet learned all of those shortcuts.

NIC Equipment Manager brings those recurring tasks together in one place. It is intended as both a productivity tool for experienced users and a workflow aid for people still becoming familiar with the underlying Windows networking tools.

## Screenshots

### Static profile workflow
![Static profile loaded](docs/screenshots/Static%20Profile%20Loaded.PNG)

### Device access and continuous ping
![Device access and continuous ping](docs/screenshots/Device%20Access-Continuous%20Ping.PNG)

### Advanced Tools
![Advanced Tools window](docs/screenshots/Advanced%20Tools%20Window.PNG)

### Responsive layout
![Responsive layout](docs/screenshots/Responsive%20Layout.PNG)

## Project Notes

This project originated from a real recurring technical workflow. Public documentation and screenshots intentionally use generic/home-lab demonstration data and omit organization-specific equipment, network topology, addresses, hostnames, and other non-public technical details.

AI-assisted coding and debugging were used as development accelerators, while requirements, workflow design, testing, validation, and acceptance were driven by hands-on use.

## Build

The project is built with CMake and Visual Studio on Windows.

From a Visual Studio Developer PowerShell or Developer Command Prompt:

```powershell
.\BUILD-WINDOWS.bat
```

The compiled executable is generated under:

```text
Source-Code\build\bin\Release\NICEquipmentManager.exe
```

## Release

The source repository intentionally does not track compiled binaries or local runtime data.

Prebuilt releases can be published through the repository's **Releases** section.

## Version

Current portfolio baseline: **v1.3.3**

Notable v1.3.3 work includes:

- Advanced Tools companion window
- Administrator shell launchers
- Integrated network diagnostics
- Free-form elevated command execution
- Responsive layout refinements
- Correct handling of disconnected NIC static/DHCP state
- Verification of applied IPv4 configuration
- Advanced Tools output repaint fix while scrolling

## Repository Structure

```text
NIC-Equipment-Manager/
├─ README.md
├─ Source-Code/
├─ docs/
│  ├─ NIC-Equipment-Manager-Case-Study.md
│  └─ screenshots/
└─ release-notes/
   └─ RELEASE-NOTES-v1.3.3.md
```

## License

No license is currently granted for reuse or redistribution of the source code.
