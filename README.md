# CampusBallot

A desktop application for running secure, offline-first campus elections — student council, class reps, club leadership, and similar votes. Built with **C++17** and **Qt 6**, using a local SQLite database by default (with optional Firebase sync).

> **Status:** Pre-release / preview build (v1.0-Beta). See [RELEASE_NOTES.md](.releases/tag/v.1.0-Beta) for what's new and current limitations.

---

## Table of contents

- [Features](#features)
- [Quick start (Windows binary)](#quick-start-windows-binary)
- [First-time setup](#first-time-setup)
- [Using the app](#using-the-app)
  - [Roles](#roles)
  - [Running an election, step by step](#running-an-election-step-by-step)
  - [The voting kiosk](#the-voting-kiosk)
  - [Results & reporting](#results--reporting)
  - [Backups](#backups)
- [Where your data lives](#where-your-data-lives)
- [Building from source](#building-from-source)
- [Project structure](#project-structure)
- [Troubleshooting](#troubleshooting)
- [Security notes](#security-notes)

---

## Features

- 🗳️ Create and run multiple elections with custom eligibility rules (by class/department)
- 👤 Student roster with photo, QR/barcode/RFID support, and one-vote-per-student enforcement
- 🧑‍🤝‍🧑 Candidate profiles with manifesto, party, symbol, and campaign poster, plus an approval step
- 🔐 Role-based access control (6 roles), AES-256 encryption, digital signatures, and tamper detection on stored data
- 📜 Immutable, hashed audit log for every login, vote, and admin action
- 💾 Scheduled, encrypted backups with checksum verification
- 📊 Live results dashboard with vote counts and percentages
- 🔌 Plugin system for extending functionality
- ☁️ Optional Firebase Realtime Database sync for multi-machine setups

---

## Quick start (Windows binary)

1. Unzip `TheRealCampusBallot.zip`.
2. Open the `windows-app` folder.
3. Double-click **`TheRealCampusBallot.exe`**.
   - Windows may show a SmartScreen warning because this build isn't code-signed yet — click **More info → Run anyway** if you trust the source.
4. On first launch, the **Setup Wizard** opens automatically. Follow the steps below.

> The `windows-app` folder is portable — everything the app needs (Qt runtime DLLs, drivers, translations) is already inside it. Keep the folder intact; don't move the `.exe` out on its own.

---

## First-time setup

The Setup Wizard runs once, the first time the app is launched (or any time a reset is triggered). It walks through:

1. **Welcome** — introduces the app.
2. **Storage selection** — choose where election data lives:
   - **Local (SQLite)** — recommended for a single machine or a small local network. No internet required. *(This is the most tested path in the current build.)*
   - **Firebase Realtime Database** — for syncing across multiple machines over the internet. You'll need a Firebase project's API key, Project ID, Database URL, and (if used) a database secret. Treat this option as experimental in the current build.
3. **Local configuration** *(if SQLite chosen)* — confirms the local database file location.
4. **Firebase configuration** *(if Firebase chosen)* — enter and validate your Firebase credentials.
5. **Admin account** — create the first **Super Administrator** account (name, email, password). This is the account you'll use to manage everything else.
6. **Summary** — review your choices before finishing setup.

Once setup completes, the main application window opens and you're logged in as the admin you just created.

**To re-run setup later** (e.g., to switch storage backends or reset the app), delete or rename `settings.ini` in the app data folder (see [Where your data lives](#where-your-data-lives)) and relaunch the app.

---

## Using the app

### Roles

The app uses role-based access control. When creating additional users (Admin Panel → User Management), assign one of:

| Role | Typical use |
|---|---|
| **Super Administrator** | Full access — created during setup, manages everything including other admins |
| **Election Administrator** | Creates/manages elections, candidates, and voting sessions |
| **Teacher** | Assists with student verification / oversight |
| **Student Volunteer** | Staffs the voting kiosk, verifies students |
| **Observer** | Read-only visibility, for transparency during voting |
| **Result Auditor** | Access to audit logs and result verification, without election-editing rights |

### Running an election, step by step

1. **Log in** as an Administrator.
2. **Add students** to the roster (Student Management) — either manually or by importing a list. Optionally add photos and set up QR/barcode/RFID identifiers.
3. **Create an election** (Admin Panel → Elections):
   - Set title, description, and voting window (start/end).
   - Restrict eligibility by class and/or department if needed.
   - Set max votes per student.
   - Decide whether student verification is required, and against what (a file column, etc.).
4. **Add candidates** to the election: name, photo, manifesto, party, symbol, and (optionally) a campaign poster or video link. Each candidate needs to be **approved** before they appear on the ballot.
5. **Start voting.** This moves the election into the `Voting` state. You can `Pause` and resume, or `End` the election when done, from the admin dashboard.
6. **Open the Voting Kiosk** on the machine(s) you want voters to use (see below).
7. **Monitor** the live dashboard while voting is open.
8. **End the election** and view results.

### The voting kiosk

The Voting Kiosk is a locked-down view meant to be shown on a shared voting terminal (a Chromebook-style setup, a booth, etc.), separate from the admin interface:

- A voter is identified (manually, or via QR/barcode/RFID scan).
- The system checks eligibility and whether they've already voted.
- The voter selects a candidate and confirms.
- The vote is hashed, signed, and recorded — the voter cannot vote again on the same election.

Run this on the machine(s) you're using as polling stations, while keeping the admin login separate on a different machine or session.

### Results & reporting

- The **Results view** shows live vote counts and percentages per candidate as votes come in (if "allow results preview" is enabled) or after the election ends.
- Results can be exported for record-keeping.

### Backups

- Configure automatic backups (Admin Panel → Settings): enable/disable, set the interval in hours, and set how many backups to retain.
- Backups are encrypted and checksummed.
- You can also trigger a manual backup at any time.
- **Recommended:** periodically copy backups off the voting machine (USB drive, network share, etc.) in case of hardware failure.

---

## Where your data lives

The app stores its working files in the standard Qt application data location for your OS/user, based on the organization/app name (`CampusBallot` / `TheRealCampusBallot`). On Windows this is typically:

```
%APPDATA%\CampusBallot\TheRealCampusBallot\
```

Inside you'll find:
- `ballot.db` — the local SQLite database (students, candidates, elections, votes, users)
- `settings.ini` — app settings, including your storage/theme configuration
- `ballot.log` — a running application log (useful for troubleshooting)

**Back this folder up** if you're not using the app's built-in backup feature, especially before major changes.

---

## Building from source

If you'd rather build from the C++ source instead of using the prebuilt Windows binary:

**Requirements:**
- CMake ≥ 3.20
- A C++17-capable compiler (MSVC, MinGW, GCC, or Clang)
- Qt 6 with these components: `Core Gui Widgets Sql Network Charts Svg PrintSupport`

**Steps:**

```bash
# from the project root (the folder containing CMakeLists.txt)
cmake -B build -S . -DCMAKE_PREFIX_PATH="<path-to-your-Qt6-install>"
cmake --build build --config Release
```

The build uses `qt_add_executable`, `CMAKE_AUTOMOC`, `CMAKE_AUTORCC`, and `CMAKE_AUTOUIC`, so Qt's meta-object/resource/UI tooling is handled automatically — you don't need to run `moc`/`rcc`/`uic` by hand.


---

## Project structure

```
TheRealCampusBallot/
├── main.cpp                    # App entry point, bootstrap, splash → wizard/main window
├── CMakeLists.txt
├── assets/                     # Branding, backgrounds, placeholder images
├── resources/                  # Icons, themes, translations (Qt resource system)
├── src/
│   ├── core/                   # Constants, data models, theme manager, bootstrap/config
│   ├── modules/
│   │   ├── auth/                # Login + role-based access control (RBAC)
│   │   ├── election/             # Election + vote management
│   │   ├── voting/kiosk/         # Voting kiosk logic
│   │   ├── student/               # Student roster, identification, import
│   │   ├── security/             # AES-256, hashing, digital signatures, tamper detection
│   │   ├── storage/               # Storage provider interface + SQLite implementation
│   │   ├── audit/                # Audit logging
│   │   ├── backup/               # Backup/restore
│   │   ├── integration/          # Firebase Realtime Database sync
│   │   └── plugin/               # Plugin loading (IPlugin interface)
│   └── ui/
│       ├── views/                # Main windows: login, dashboard, admin panel, kiosk, etc.
│       ├── viewmodels/
│       ├── components/, widgets/, dialogs/, styles/
└── windows-app/                 # Prebuilt Windows x64 binary + Qt runtime dependencies
```

---

## Troubleshooting

**The app won't start / closes immediately**
Check `ballot.log` in the app data folder (see above) for the last error before the crash.

**Windows says "Windows protected your PC"**
This is SmartScreen reacting to an unsigned executable, not a detected threat. Click **More info → Run anyway** if you obtained the file from a source you trust.

**I want to start over / setup is stuck**
Close the app, delete `settings.ini` from the app data folder, and relaunch — this triggers the Setup Wizard again. (This does **not** delete `ballot.db`, so existing election data is preserved unless you delete that too.)

**Firebase setup won't validate**
Double-check the API key, Project ID, and Database URL, and confirm your Firebase project's Realtime Database is actually enabled. As noted in the release notes, this path is less tested than local SQLite in the current build.

---

## Security notes

- Passwords are stored as a hash+salt, never in plaintext.
- Votes are hashed and digitally signed at the point of casting.
- Audit log entries are hashed to make tampering detectable.
- All of the above is implemented and active, but **this build has not had an independent third-party security audit** — see [RELEASE_NOTES.md](./RELEASE_NOTES.md) for details before relying on it for a high-stakes or legally binding election.

---
