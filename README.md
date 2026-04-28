<img width="1211" height="1294" alt="123" src="https://github.com/user-attachments/assets/903838ed-399a-4cf4-8ef6-2e01bd1945a5" />


# TopWindow - Android UI Hierarchy Viewer

A GTK4 application to view and inspect Android UI hierarchy dumps.

## Features

- Connects to Android devices via adb
- Generates UI hierarchy dumps using uiautomator
- Displays hierarchy in an expandable tree view
- Shows detailed node attributes
- Caches dumps for offline viewing

## Dependencies

Build:
- C++17 compiler
- Meson build system
- Ninja
- GTK4 development files
- libxml2 development files

Runtime:
- adb (Android Debug Bridge)
- Android device with USB debugging enabled

## Building

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt install meson ninja-build libgtk-4-dev libxml2-dev g++

# Build
meson setup build
cd build
ninja

# Run
./topwindow
```

## Usage

1. Connect your Android device with USB debugging enabled
2. Run `./topwindow`
3. Click "Start" to fetch the current UI hierarchy
4. Navigate the tree view - click a node to see its attributes
5. Next time you can view cached data without reconnecting

## Project Structure

```
src/
├── main.cpp                  - Entry point
├── core/
│   ├── AdbManager.cpp/hpp    - adb command execution
│   ├── CacheManager.cpp/hpp  - cache file management
│   └── XmlParser.cpp/hpp     - XML parsing using libxml2
├── models/
│   ├── UiNode.cpp/hpp        - tree node data structure
│   └── UiTreeModel.cpp/hpp   - custom GtkTreeModel
└── ui/
    ├── MainWindow.cpp/hpp    - main application window
    ├── HierarchyWindow.cpp/hpp - tree view window
    └── NodeAttributesPanel.cpp/hpp - node attributes panel
```
