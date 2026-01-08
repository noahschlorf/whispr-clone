# Ralph Wiggum Plan: Menu Bar Icon Implementation

## Overview

Replace emoji-based menu bar icons (🎤, 🔴, ⏳, ⚠️) with proper image-based icons that match the macOS menu bar aesthetic (like Ollama's llama icon shown in reference screenshot).

## Current State

- **Location**: `/Users/noahschlorf/claude/whispr-clone/src/platform/macos/tray_macos.mm`
- **Current Implementation**: Unicode emoji strings set via `g_status_item.button.title`
- **States**: Idle (🎤), Recording (🔴), Transcribing (⏳), Error (⚠️)
- **Problem**: Emoji icons look inconsistent with native macOS menu bar apps

## Target State

- Professional template images (PDF vector or @1x/@2x PNG) for all 4 states
- Icons render correctly in both light and dark mode
- Icons match the visual style of native macOS menu bar apps
- Clean integration with existing state machine

---

## Phase 1: Create Icon Assets

### Tasks
1. Create `assets/` directory in project root
2. Design/create 4 icon states as macOS template images:
   - `whispr-idle.pdf` - Microphone icon (ready state)
   - `whispr-recording.pdf` - Microphone with recording indicator
   - `whispr-transcribing.pdf` - Microphone with processing indicator
   - `whispr-error.pdf` - Microphone with error indicator
3. Icons should be:
   - 18x18 points (36x36 @2x pixels)
   - Single color (black) - macOS templates auto-tint for dark/light mode
   - PDF vector format preferred for retina scaling

### Verification
- [ ] `assets/` directory exists
- [ ] All 4 PDF/PNG icon files present
- [ ] Icons are proper dimensions (18x18 pt)

---

## Phase 2: Update CMake Build Configuration

### Tasks
1. Add `assets/` directory to CMakeLists.txt resource copying
2. Ensure icons are bundled in `Whispr.app/Contents/Resources/`
3. Update build script if needed

### Verification
- [ ] Build completes without errors
- [ ] Icons present in built app bundle at `build/Whispr.app/Contents/Resources/`

---

## Phase 3: Modify tray_macos.mm Implementation

### Tasks
1. Replace emoji-based `button.title` with `button.image` using NSImage
2. Load icons from app bundle Resources
3. Set `template = YES` on images for auto dark/light mode
4. Update `update_tray_state()` function to swap images based on state
5. Add fallback to emoji if image loading fails

### Code Changes Required

**Current Code** (`tray_macos.mm`):
```objc
void update_tray_state(AppState state) {
    NSString *icon = @"🎤";
    // ...
    g_status_item.button.title = icon;
}
```

**Target Code**:
```objc
NSImage* load_tray_icon(NSString* name) {
    NSBundle *bundle = [NSBundle mainBundle];
    NSString *path = [bundle pathForResource:name ofType:@"pdf"];
    NSImage *image = [[NSImage alloc] initWithContentsOfFile:path];
    [image setTemplate:YES];
    [image setSize:NSMakeSize(18, 18)];
    return image;
}

void update_tray_state(AppState state) {
    NSString *iconName = @"whispr-idle";
    NSString *status = @"Whispr - Ready";

    switch (state) {
        case AppState::Idle:
            iconName = @"whispr-idle";
            status = @"Whispr - Ready";
            break;
        case AppState::Recording:
            iconName = @"whispr-recording";
            status = @"Recording...";
            break;
        case AppState::Transcribing:
            iconName = @"whispr-transcribing";
            status = @"Transcribing...";
            break;
        case AppState::Error:
            iconName = @"whispr-error";
            status = @"Error";
            break;
    }

    NSImage *icon = load_tray_icon(iconName);
    if (icon) {
        g_status_item.button.image = icon;
        g_status_item.button.title = @"";
    } else {
        // Fallback to emoji
        g_status_item.button.image = nil;
        g_status_item.button.title = @"🎤";
    }
}
```

### Verification
- [ ] Code compiles without errors
- [ ] Icons load successfully at runtime
- [ ] State transitions show correct icon
- [ ] Icons render correctly in light mode
- [ ] Icons render correctly in dark mode

---

## Phase 4: Integration Testing

### Tasks
1. Build the application: `./build.sh`
2. Run the app and verify menu bar icon appears
3. Test all 4 states:
   - Launch app → Idle icon visible
   - Hold hotkey → Recording icon visible
   - Release hotkey → Transcribing icon visible (briefly)
   - Simulate error → Error icon visible
4. Toggle macOS dark/light mode and verify icon adapts
5. Compare appearance to native apps (Ollama, etc.)

### Verification
- [ ] App builds successfully
- [ ] Menu bar icon visible on launch
- [ ] All state icons display correctly
- [ ] Dark/light mode auto-tinting works
- [ ] No console errors about missing resources

---

## Phase 5: Cleanup and Polish

### Tasks
1. Remove any unused emoji code paths (unless keeping as fallback)
2. Update README.md with icon asset information
3. Add comments to new icon loading code
4. Verify no memory leaks from NSImage allocations

### Verification
- [ ] Code is clean and documented
- [ ] No memory leaks
- [ ] README updated if needed

---

## Completion Criteria

All of the following must be true:
- [ ] 4 icon assets created and properly formatted
- [ ] Icons bundled in app at build time
- [ ] Menu bar shows image icon (not emoji) on app launch
- [ ] All 4 states display their corresponding icon
- [ ] Icons auto-tint for dark/light mode via template mode
- [ ] Build passes without warnings
- [ ] App runs without errors

---

## Ralph Loop Execution Command

```bash
/ralph-loop "Implement menu bar icon for whispr-clone following RALPH_WIGGUM_PLAN.md.

Current: Emoji icons (🎤🔴⏳⚠️) in macOS menu bar
Target: Proper template images like native macOS apps

Execute phases 1-5 in order:
1. Create assets/ dir with 4 PDF template icons (18x18pt, black, single color)
2. Update CMakeLists.txt to bundle icons in Resources
3. Modify tray_macos.mm to use NSImage with template:YES
4. Build and verify all states work
5. Clean up code

After each phase, verify the checklist items before proceeding.

Output <promise>ICON_IMPLEMENTATION_COMPLETE</promise> when:
- All 4 icons created and bundled
- tray_macos.mm uses NSImage instead of emoji
- App builds and runs with proper menu bar icons
- Dark/light mode auto-tinting works

If blocked after 15 iterations:
- Document blocking issue
- List what was attempted
- Suggest manual steps needed" --completion-promise "ICON_IMPLEMENTATION_COMPLETE" --max-iterations 50
```

---

## Escape Hatches

### If icon creation is blocked:
- Use SF Symbols as alternative (NSImage imageWithSystemSymbolName)
- Use simple geometric shapes drawn programmatically
- Keep emoji fallback as temporary solution

### If CMake bundling fails:
- Manual copy icons to Resources folder for testing
- Check Info.plist for resource configuration issues

### If NSImage loading fails:
- Verify bundle path is correct
- Check file permissions
- Try PNG format instead of PDF
- Add detailed logging to diagnose

---

## Files to Create/Modify

| File | Action | Description |
|------|--------|-------------|
| `assets/whispr-idle.pdf` | CREATE | Idle state icon |
| `assets/whispr-recording.pdf` | CREATE | Recording state icon |
| `assets/whispr-transcribing.pdf` | CREATE | Transcribing state icon |
| `assets/whispr-error.pdf` | CREATE | Error state icon |
| `CMakeLists.txt` | MODIFY | Add asset bundling |
| `src/platform/macos/tray_macos.mm` | MODIFY | Use NSImage |

---

## Notes

- macOS template images should be single-color black; the system auto-tints for dark mode
- PDF format is preferred over PNG for crisp retina rendering
- NSStatusItem button can have either `title` OR `image`, not both simultaneously for best results
- Consider using SF Symbols if custom icons prove difficult (e.g., `mic.fill`, `mic.circle`, etc.)
