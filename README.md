# DeltaSync
DeltaSync is a lightweight, simplified version control system designed for local networks. It allows users to manage file versions, track changes, and synchronize updates efficiently using byte-level diffs.

- Features
  - Version Management : Store full file versions on initial upload and only byte differences (deltas) for subsequent changes.
  - Branching Support : Automatically create new branches when previous versions are modified.
  - History Tracking : View the complete history of changes and branch structures.
  - Efficient Storage : Minimize storage usage by saving only the differences between file versions.
  - Network Capabilities : Handle multiple client connections asynchronously using Boost.Asio.
  - Thread Safety : Ensure safe concurrent access with mutex-based synchronization.
  - Command-Line Configuration : Easily configure the server via command-line arguments.

- Use Cases
  - Collaborative projects in local networks.
  - Lightweight version control for small teams.
  - Educational purposes to understand version control systems.

- Prerequisites
  - C++20 compiler
  - Boost.Asio library
  - CMake
