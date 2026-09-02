CampusBallot

What if running a student election were as secure as a real one? CampusBallot is a desktop application for running secure, offline-first campus elections — student council, class reps, club leadership, and similar votes. It includes role-based access, encrypted vote storage, a dedicated voting kiosk, and live results. All of it runs locally, with optional cloud sync.

Status: Pre-release/preview build (v1.0-Beta). See RELEASE_NOTES.md for what's new and current limitations.

Screenshots:

A few glimpses of CampusBallot are below, but they only show part of the story. The real experience is within the app. Explore, click around, and discover the tools you'll find.

1. Setup Wizard:

(screenshot placeholder)

2. Admin Dashboard:

(screenshot placeholder)

3. Voting Kiosk:

(screenshot placeholder)

4. Live Results View:

(screenshot placeholder)

Try It
<p align="center"> <strong>Download the Windows binary below to launch CampusBallot instantly.</strong><br> See <a href="#quick-start-windows-binary">Quick Start</a> for setup steps. </p>
Quick Start (Windows binary)
Unzip TheRealCampusBallot.zip.
Open the windows-app folder.
Double-click TheRealCampusBallot.exe.
Windows may show a SmartScreen warning because this build isn't code-signed yet — click More info → Run anyway if you trust the source.
On first launch, the Setup Wizard opens automatically and walks you through storage selection, configuration, and creating your first admin account.

The windows-app folder is portable — everything the app needs (Qt runtime DLLs, drivers, translations) is already inside it. Keep the folder intact; don't move the .exe out on its own.
Ṅ
Features
🗳️ Create and run multiple elections with custom eligibility rules (by class/department)
👤 Student roster with photo, QR/barcode/RFID support, and one-vote-per-student enforcement
🧑‍🤝‍🧑 Candidate profiles with manifesto, party, symbol, and campaign poster, plus an approval step
🔐 Role-based access control (6 roles), AES-256 encryption, digital signatures, and tamper detection on stored data
📜 Immutable, hashed audit log for every login, vote, and admin action
💾 Scheduled, encrypted backups with checksum verification
📊 Live results dashboard with vote counts and percentages
🔌 Plugin system for extending functionality
☁️ Optional Firebase Realtime Database sync for multi-machine setups
🖥️ Dedicated, lockable Voting Kiosk view separate from the admin interface
🌐 Fully offline-first — no internet required for local (SQLite) setups
Roles

CampusBallot uses role-based access control. When creating additional users (Admin Panel → User Management), assign one of:

Role	Typical use
Super Administrator	Full access — created during setup, manages everything including other admins
Election Administrator	Creates/manages elections, candidates, and voting sessions
Teacher	Assists with student verification / oversight
Student Volunteer	Staffs the voting kiosk, verifies students
Observer	Read-only visibility, for transparency during voting
Result Auditor	Access to audit logs and result verification, without election-editing rights
Running an Election, Step by Step
Log in as an Administrator.
Add students to the roster (Student Management) — either manually or by importing a list. Optionally add photos and set up QR/barcode/RFID identifiers.
Create an election (Admin Panel → Elections) — set title, description, voting window, eligibility rules, and max votes per student.
Add candidates — name, photo, manifesto, party, symbol, and (optionally) a campaign poster or video link. Each candidate needs to be approved before appearing on the ballot.
Start voting. The election moves into the Voting state. You can Pause, resume, or End it from the admin dashboard.
Open the Voting Kiosk on the machine(s) you want voters to use.
Monitor the live dashboard while voting is open.
End the election and view results.
🛠️ Building From Source
Requirements
CMake ≥ 3.20
A C++17-capable compiler (MSVC, MinGW, GCC, or Clang)
Qt 6 with these components: Core Gui Widgets Sql Network Charts Svg PrintSupport
🛠️ Building
bash
# from the project root (the folder containing CMakeLists.txt)
cmake -B build -S . -DCMAKE_PREFIX_PATH="<path-to-your-Qt6-install>"
cmake --build build --config Release

The build uses qt_add_executable, CMAKE_AUTOMOC, CMAKE_AUTORCC, and CMAKE_AUTOUIC, so Qt's meta-object/resource/UI tooling is handled automatically — you don't need to run moc/rcc/uic by hand.

Project Structure
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
Where Your Data Lives

CampusBallot stores its working files in the standard Qt application data location for your OS/user, based on the organization/app name (CampusBallot / TheRealCampusBallot). On Windows this is typically:

%APPDATA%\CampusBallot\TheRealCampusBallot\

Inside you'll find:

ballot.db — the local SQLite database (students, candidates, elections, votes, users)
settings.ini — app settings, including your storage/theme configuration
ballot.log — a running application log (useful for troubleshooting)

Back this folder up if you're not using the app's built-in backup feature, especially before major changes.

Troubleshooting

The app won't start / closes immediately Check ballot.log in the app data folder (see above) for the last error before the crash.

Windows says "Windows protected your PC" This is SmartScreen reacting to an unsigned executable, not a detected threat. Click More info → Run anyway if you obtained the file from a source you trust.

I want to start over / setup is stuck Close the app, delete settings.ini from the app data folder, and relaunch — this triggers the Setup Wizard again. (This does not delete ballot.db, so existing election data is preserved unless you delete that too.)

Firebase setup won't validate Double-check the API key, Project ID, and Database URL, and confirm your Firebase project's Realtime Database is actually enabled. This path is less tested than local SQLite in the current build.

Security Notes
Passwords are stored as a hash+salt, never in plaintext.
Votes are hashed and digitally signed at the point of casting.
Audit log entries are hashed to make tampering detectable.
All of the above is implemented and active, but this build has not had an independent third-party security audit — see RELEASE_NOTES.md for details before relying on it for a high-stakes or legally binding election.
Credits

Built with C++17 and Qt 6, using a local SQLite database by default (with optional Firebase sync).

🗺️ Future Plans

CampusBallot is still actively being developed. Some ideas currently on the roadmap include:

Independent third-party security audit
Code-signed Windows releases
Improved Firebase sync reliability
Mobile-friendly voting kiosk mode
Additional plugin hooks
More granular reporting/export formats
