# Ralph Wiggum Plan: VoxType Rebranding & Menu Bar Features

## Overview

Transform whispr-clone into VoxType with enhanced menu bar functionality.

## Features to Implement

### 1. Rename App to VoxType
- Change binary name from `whispr` to `voxtype`
- Update all user-facing strings
- Update window titles, menu items, logs

### 2. Click-to-Toggle Functionality
- Click menu bar icon to enable/disable hotkey listening
- When disabled, hotkey presses are ignored
- State persists visually in icon

### 3. Grayed Icon for Disabled State
- Normal icon when enabled (current icons)
- Grayed/dim version when disabled
- Clear visual distinction

### 4. Transcription History in Menu
- Store last 5 transcriptions
- Show in dropdown menu
- Click to copy to clipboard
- Truncate long text with "..."

### 5. Recording Indicator
- Icon changes color/style while recording
- Different icon while transcribing
- Clear feedback for current state

---

## Phase 1: Rename to VoxType

### Tasks
1. Update CMakeLists.txt - change target name to `voxtype`
2. Update main.cpp - change app name in output
3. Update tray_macos.mm - change menu title and tooltip
4. Update any hardcoded "whispr" strings

### Files to Modify
- `CMakeLists.txt`
- `src/main.cpp`
- `src/platform/macos/tray_macos.mm`

### Verification
- [ ] Binary builds as `voxtype`
- [ ] Menu bar shows "VoxType"
- [ ] Console output says "VoxType"

---

## Phase 2: Click-to-Toggle

### Tasks
1. Add `enabled_` state to App class
2. Modify tray click handler to toggle state
3. Skip hotkey processing when disabled
4. Update icon based on state

### Files to Modify
- `include/app.hpp` - add enabled_ member
- `src/app.cpp` - add toggle logic
- `src/platform/macos/tray_macos.mm` - handle click event

### Implementation
```cpp
// In app.hpp
class App {
    // ...
    std::atomic<bool> enabled_{true};
public:
    void toggle_enabled();
    bool is_enabled() const { return enabled_.load(); }
};

// In app.cpp
void App::toggle_enabled() {
    enabled_.store(!enabled_.load());
    update_tray_state(enabled_.load() ? AppState::Idle : AppState::Disabled);
}

void App::on_hotkey(bool pressed) {
    if (!enabled_.load()) return;  // Skip if disabled
    // ... existing code
}
```

### Verification
- [ ] Clicking icon toggles enabled state
- [ ] Hotkey ignored when disabled
- [ ] State shown in icon

---

## Phase 3: Grayed Disabled Icon

### Tasks
1. Create grayed version of idle icon
2. Add AppState::Disabled state
3. Update icon when toggling

### Files to Modify
- `include/app.hpp` - add Disabled state
- `src/platform/macos/tray_macos.mm` - add disabled icon

### Implementation
```objc
// Create grayed icon
NSImage* createDisabledIcon() {
    NSImage* icon = createIdleIcon();
    // Apply grayscale filter or reduce alpha
    [icon lockFocus];
    [[NSColor colorWithWhite:0.5 alpha:0.5] set];
    NSRectFillUsingOperation(NSMakeRect(0, 0, 22, 22), NSCompositingOperationSourceAtop);
    [icon unlockFocus];
    return icon;
}
```

### Verification
- [ ] Disabled icon is visibly different
- [ ] Icon updates immediately on toggle
- [ ] Icon is still recognizable

---

## Phase 4: Transcription History Menu

### Tasks
1. Add history storage (vector of last 5)
2. Create dropdown menu with history items
3. Add click handler to copy text
4. Truncate long transcriptions

### Files to Modify
- `include/app.hpp` - add history storage
- `src/app.cpp` - store transcriptions
- `src/platform/macos/tray_macos.mm` - create menu

### Implementation
```cpp
// In app.hpp
class App {
    std::vector<std::string> transcription_history_;
    static constexpr size_t MAX_HISTORY = 5;
public:
    const std::vector<std::string>& get_history() const;
    void add_to_history(const std::string& text);
};

// In app.cpp
void App::add_to_history(const std::string& text) {
    transcription_history_.insert(transcription_history_.begin(), text);
    if (transcription_history_.size() > MAX_HISTORY) {
        transcription_history_.pop_back();
    }
}
```

### Menu Structure
```
[VoxType Icon]
├── ✓ Enabled / ○ Disabled  (toggle)
├── ─────────────
├── Recent:
│   ├── "Hello this is a test..."
│   ├── "Another transcription..."
│   └── (up to 5 items)
├── ─────────────
└── Quit VoxType
```

### Verification
- [ ] Menu shows last 5 transcriptions
- [ ] Long text truncated with "..."
- [ ] Clicking copies to clipboard
- [ ] History persists during session

---

## Phase 5: Recording Indicator

### Tasks
1. Create animated/colored recording icon
2. Create transcribing icon (different from recording)
3. Ensure smooth transitions

### Current States
- Idle: Normal microphone icon
- Recording: Red/pulsing icon
- Transcribing: Processing indicator
- Disabled: Grayed icon
- Error: Red X icon

### Files to Modify
- `src/platform/macos/tray_macos.mm`

### Verification
- [ ] Icon clearly shows recording state
- [ ] Icon clearly shows transcribing state
- [ ] Transitions are smooth
- [ ] No flickering

---

## Completion Criteria

All of the following must be true:
- [ ] App renamed to VoxType
- [ ] Click icon toggles enabled/disabled
- [ ] Grayed icon when disabled
- [ ] Menu shows last 5 transcriptions
- [ ] Recording/transcribing states visible
- [ ] All features work without crashes
- [ ] Build succeeds

---

## Ralph Loop Execution

```
Execute phases in order:
1. Rename app to VoxType
2. Add click-to-toggle functionality
3. Add grayed disabled icon
4. Add transcription history menu
5. Add recording indicator

After each phase:
- Build and test
- Verify checklist items
- Commit changes

Output VOXTYPE_COMPLETE when all features working.
```

---

## Files Summary

| File | Changes |
|------|---------|
| CMakeLists.txt | Rename target |
| src/main.cpp | Update app name |
| include/app.hpp | Add enabled_, history |
| src/app.cpp | Toggle logic, history storage |
| src/platform/macos/tray_macos.mm | Menu, icons, click handler |
