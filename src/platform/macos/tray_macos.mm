#import <Cocoa/Cocoa.h>
#include "app.hpp"
#include <iostream>
#include <vector>
#include <string>

static whispr::App* g_app = nil;
static NSStatusItem* g_status_item = nil;
static BOOL g_status_ready = NO;
static BOOL g_enabled = YES;

// Transcription history
static std::vector<std::string> g_history;
static const size_t MAX_HISTORY = 5;

// Cache SF Symbol images for performance
static NSImage* g_icon_idle = nil;
static NSImage* g_icon_recording = nil;
static NSImage* g_icon_transcribing = nil;
static NSImage* g_icon_error = nil;
static NSImage* g_icon_disabled = nil;

// Load SF Symbol as template image for menu bar
static NSImage* loadSFSymbol(NSString* symbolName) {
    if (@available(macOS 11.0, *)) {
        NSImageSymbolConfiguration* config = [NSImageSymbolConfiguration
            configurationWithPointSize:16
            weight:NSFontWeightRegular
            scale:NSImageSymbolScaleMedium];
        NSImage* image = [NSImage imageWithSystemSymbolName:symbolName accessibilityDescription:nil];
        if (image) {
            image = [image imageWithSymbolConfiguration:config];
            [image setTemplate:YES];  // Enable dark/light mode auto-tinting
            return image;
        }
    }
    return nil;
}

// Create grayed out version of an icon
static NSImage* createGrayedIcon(NSImage* sourceIcon) {
    if (!sourceIcon) return nil;

    NSSize size = sourceIcon.size;
    NSImage* grayIcon = [[NSImage alloc] initWithSize:size];

    [grayIcon lockFocus];
    [sourceIcon drawInRect:NSMakeRect(0, 0, size.width, size.height)
                  fromRect:NSZeroRect
                 operation:NSCompositingOperationSourceOver
                  fraction:0.3];  // 30% opacity for grayed look
    [grayIcon unlockFocus];

    [grayIcon setTemplate:YES];
    return grayIcon;
}

// Initialize all icons (called once on app start)
static void initializeIcons() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        g_icon_idle = loadSFSymbol(@"mic.fill");
        g_icon_recording = loadSFSymbol(@"mic.circle.fill");
        g_icon_transcribing = loadSFSymbol(@"waveform");
        g_icon_error = loadSFSymbol(@"exclamationmark.triangle.fill");

        // Create disabled icon (grayed mic with slash)
        g_icon_disabled = loadSFSymbol(@"mic.slash.fill");
        if (!g_icon_disabled && g_icon_idle) {
            g_icon_disabled = createGrayedIcon(g_icon_idle);
        }

        // Retain icons so they persist
        if (g_icon_idle) [g_icon_idle retain];
        if (g_icon_recording) [g_icon_recording retain];
        if (g_icon_transcribing) [g_icon_transcribing retain];
        if (g_icon_error) [g_icon_error retain];
        if (g_icon_disabled) [g_icon_disabled retain];

        NSLog(@"Icons initialized - idle:%@ recording:%@ transcribing:%@ error:%@ disabled:%@",
              g_icon_idle ? @"OK" : @"FAIL",
              g_icon_recording ? @"OK" : @"FAIL",
              g_icon_transcribing ? @"OK" : @"FAIL",
              g_icon_error ? @"OK" : @"FAIL",
              g_icon_disabled ? @"OK" : @"FAIL");
    });
}

// Track current quality mode
static whispr::ModelQuality g_current_quality = whispr::ModelQuality::Balanced;

@interface VoxTypeAppDelegate : NSObject <NSApplicationDelegate>
- (void)updateQualityMenu;
- (void)updateHistoryMenu;
- (void)updateEnabledState;
@end

@implementation VoxTypeAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification {
    (void)notification;

    // Initialize SF Symbol icons
    initializeIcons();

    // Create status bar item on main thread after app launches
    dispatch_async(dispatch_get_main_queue(), ^{
        g_status_item = [[NSStatusBar systemStatusBar] statusItemWithLength:NSVariableStatusItemLength];

        if (g_status_item) {
            // Use SF Symbol icon, fallback to emoji if unavailable
            if (g_icon_idle) {
                g_status_item.button.image = g_icon_idle;
                g_status_item.button.title = @"";
            } else {
                g_status_item.button.title = @"🎤";
            }

            // Add click action for toggle
            g_status_item.button.action = @selector(toggleEnabled:);
            g_status_item.button.target = self;
            // Right-click or ctrl-click shows menu
            [g_status_item.button sendActionOn:NSEventMaskLeftMouseUp];

            NSMenu *menu = [[NSMenu alloc] init];

            // Status item (shows enabled/disabled)
            NSMenuItem *statusItem = [[NSMenuItem alloc] initWithTitle:@"VoxType - Ready" action:nil keyEquivalent:@""];
            statusItem.enabled = NO;
            statusItem.tag = 100;
            [menu addItem:statusItem];

            [menu addItem:[NSMenuItem separatorItem]];

            // Enable/Disable toggle
            NSMenuItem *enableItem = [[NSMenuItem alloc] initWithTitle:@"Enabled" action:@selector(toggleEnabled:) keyEquivalent:@""];
            enableItem.target = self;
            enableItem.tag = 150;
            enableItem.state = NSControlStateValueOn;
            [menu addItem:enableItem];

            [menu addItem:[NSMenuItem separatorItem]];

            // History section header
            NSMenuItem *historyHeader = [[NSMenuItem alloc] initWithTitle:@"Recent Transcriptions:" action:nil keyEquivalent:@""];
            historyHeader.enabled = NO;
            historyHeader.tag = 400;
            [menu addItem:historyHeader];

            // Placeholder for history items (tags 401-405)
            for (int i = 0; i < MAX_HISTORY; i++) {
                NSMenuItem *historyItem = [[NSMenuItem alloc] initWithTitle:@"" action:@selector(copyHistoryItem:) keyEquivalent:@""];
                historyItem.target = self;
                historyItem.tag = 401 + i;
                historyItem.hidden = YES;
                [menu addItem:historyItem];
            }

            // "No history" placeholder
            NSMenuItem *noHistoryItem = [[NSMenuItem alloc] initWithTitle:@"  (none yet)" action:nil keyEquivalent:@""];
            noHistoryItem.enabled = NO;
            noHistoryItem.tag = 410;
            [menu addItem:noHistoryItem];

            [menu addItem:[NSMenuItem separatorItem]];

            // Quality submenu
            NSMenuItem *qualityMenuItem = [[NSMenuItem alloc] initWithTitle:@"Quality" action:nil keyEquivalent:@""];
            NSMenu *qualityMenu = [[NSMenu alloc] init];

            NSMenuItem *fastItem = [[NSMenuItem alloc] initWithTitle:@"Fast (tiny.en)" action:@selector(setQualityFast:) keyEquivalent:@""];
            fastItem.target = self;
            fastItem.tag = 200;
            [qualityMenu addItem:fastItem];

            NSMenuItem *balancedItem = [[NSMenuItem alloc] initWithTitle:@"Balanced (base.en)" action:@selector(setQualityBalanced:) keyEquivalent:@""];
            balancedItem.target = self;
            balancedItem.tag = 201;
            balancedItem.state = NSControlStateValueOn;  // Default
            [qualityMenu addItem:balancedItem];

            NSMenuItem *accurateItem = [[NSMenuItem alloc] initWithTitle:@"Accurate (small.en)" action:@selector(setQualityAccurate:) keyEquivalent:@""];
            accurateItem.target = self;
            accurateItem.tag = 202;
            [qualityMenu addItem:accurateItem];

            NSMenuItem *bestItem = [[NSMenuItem alloc] initWithTitle:@"Best (medium.en)" action:@selector(setQualityBest:) keyEquivalent:@""];
            bestItem.target = self;
            bestItem.tag = 203;
            [qualityMenu addItem:bestItem];

            qualityMenuItem.submenu = qualityMenu;
            qualityMenuItem.tag = 300;
            [menu addItem:qualityMenuItem];

            [menu addItem:[NSMenuItem separatorItem]];

            // Quit item
            NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit VoxType" action:@selector(quitApp:) keyEquivalent:@"q"];
            quitItem.target = self;
            [menu addItem:quitItem];

            g_status_item.menu = menu;

            // Force the status item to be visible
            g_status_item.visible = YES;
            g_status_ready = YES;

            NSLog(@"Status bar item created successfully");
        } else {
            NSLog(@"Failed to create status bar item");
        }
    });
}

- (void)toggleEnabled:(id)sender {
    (void)sender;
    g_enabled = !g_enabled;

    if (g_app) {
        g_app->set_enabled(g_enabled);
    }

    [self updateEnabledState];
    NSLog(@"VoxType %@", g_enabled ? @"enabled" : @"disabled");
}

- (void)updateEnabledState {
    if (!g_status_ready || !g_status_item) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        // Update icon
        NSStatusBarButton* button = g_status_item.button;
        if (button) {
            if (g_enabled) {
                button.image = g_icon_idle;
            } else {
                button.image = g_icon_disabled;
            }
        }

        // Update menu item
        NSMenuItem *enableItem = [g_status_item.menu itemWithTag:150];
        if (enableItem) {
            enableItem.state = g_enabled ? NSControlStateValueOn : NSControlStateValueOff;
            enableItem.title = g_enabled ? @"Enabled" : @"Disabled";
        }

        // Update status text
        NSMenuItem *statusItem = [g_status_item.menu itemWithTag:100];
        if (statusItem) {
            statusItem.title = g_enabled ? @"VoxType - Ready" : @"VoxType - Disabled";
        }
    });
}

- (void)copyHistoryItem:(id)sender {
    NSMenuItem *item = (NSMenuItem *)sender;
    NSInteger index = item.tag - 401;

    if (index >= 0 && index < (NSInteger)g_history.size()) {
        NSString *text = [NSString stringWithUTF8String:g_history[index].c_str()];
        [[NSPasteboard generalPasteboard] clearContents];
        [[NSPasteboard generalPasteboard] setString:text forType:NSPasteboardTypeString];
        NSLog(@"Copied to clipboard: %@", text);
    }
}

- (void)updateHistoryMenu {
    if (!g_status_ready || !g_status_item) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        NSMenu *menu = g_status_item.menu;
        if (!menu) return;

        // Update history items
        for (size_t i = 0; i < MAX_HISTORY; i++) {
            NSMenuItem *item = [menu itemWithTag:401 + i];
            if (!item) continue;

            if (i < g_history.size()) {
                std::string text = g_history[i];
                // Truncate long text
                if (text.length() > 40) {
                    text = text.substr(0, 37) + "...";
                }
                item.title = [NSString stringWithFormat:@"  %s", text.c_str()];
                item.hidden = NO;
            } else {
                item.hidden = YES;
            }
        }

        // Show/hide "no history" placeholder
        NSMenuItem *noHistoryItem = [menu itemWithTag:410];
        if (noHistoryItem) {
            noHistoryItem.hidden = !g_history.empty();
        }
    });
}

- (void)updateQualityMenu {
    if (!g_status_item || !g_status_item.menu) return;

    NSMenuItem *qualityMenuItem = [g_status_item.menu itemWithTag:300];
    if (!qualityMenuItem || !qualityMenuItem.submenu) return;

    // Reset all checkmarks
    for (NSMenuItem *item in qualityMenuItem.submenu.itemArray) {
        item.state = NSControlStateValueOff;
    }

    // Set current checkmark
    NSInteger tag = 200 + static_cast<NSInteger>(g_current_quality);
    NSMenuItem *currentItem = [qualityMenuItem.submenu itemWithTag:tag];
    if (currentItem) {
        currentItem.state = NSControlStateValueOn;
    }

    // Update quality name in main menu
    const char* qualityName = whispr::get_profile(g_current_quality).name;
    qualityMenuItem.title = [NSString stringWithFormat:@"Quality: %s", qualityName];
}

- (void)setQualityFast:(id)sender {
    (void)sender;
    g_current_quality = whispr::ModelQuality::Fast;
    [self updateQualityMenu];
    NSLog(@"Quality set to Fast");
}

- (void)setQualityBalanced:(id)sender {
    (void)sender;
    g_current_quality = whispr::ModelQuality::Balanced;
    [self updateQualityMenu];
    NSLog(@"Quality set to Balanced");
}

- (void)setQualityAccurate:(id)sender {
    (void)sender;
    g_current_quality = whispr::ModelQuality::Accurate;
    [self updateQualityMenu];
    NSLog(@"Quality set to Accurate");
}

- (void)setQualityBest:(id)sender {
    (void)sender;
    g_current_quality = whispr::ModelQuality::Best;
    [self updateQualityMenu];
    NSLog(@"Quality set to Best");
}

- (void)quitApp:(id)sender {
    (void)sender;
    if (g_app) {
        g_app->quit();
    }
    [[NSApplication sharedApplication] terminate:nil];
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    (void)sender;
    return NO;
}

@end

static VoxTypeAppDelegate* g_delegate = nil;

namespace whispr {

bool create_tray_icon(App* app) {
    g_app = app;
    return true;
}

void destroy_tray_icon() {
    if (g_status_item) {
        [[NSStatusBar systemStatusBar] removeStatusItem:g_status_item];
        g_status_item = nil;
    }
    g_app = nil;
}

void add_to_history(const std::string& text) {
    if (text.empty()) return;

    // Add to front
    g_history.insert(g_history.begin(), text);

    // Keep max size
    if (g_history.size() > MAX_HISTORY) {
        g_history.pop_back();
    }

    // Update menu
    if (g_delegate) {
        [g_delegate updateHistoryMenu];
    }
}

void update_tray_state(AppState state) {
    if (!g_status_ready) return;

    // Capture state for the block
    AppState captured_state = state;

    dispatch_async(dispatch_get_main_queue(), ^{
        // Re-check on main thread and get a strong local reference
        if (!g_status_ready) return;

        NSStatusItem* statusItem = g_status_item;
        if (!statusItem || ![statusItem isKindOfClass:[NSStatusItem class]]) {
            NSLog(@"Warning: Invalid status item in update_tray_state");
            return;
        }

        NSStatusBarButton* button = statusItem.button;
        if (!button) {
            NSLog(@"Warning: No button on status item");
            return;
        }

        // If disabled, always show disabled icon
        if (!g_enabled) {
            button.image = g_icon_disabled;
            return;
        }

        NSImage* icon = nil;
        NSString* fallbackEmoji = @"🎤";
        NSString* statusText = @"VoxType - Ready";

        switch (captured_state) {
            case AppState::Idle:
                icon = g_icon_idle;
                fallbackEmoji = @"🎤";
                statusText = @"VoxType - Ready";
                break;
            case AppState::Recording:
                icon = g_icon_recording;
                fallbackEmoji = @"🔴";
                statusText = @"Recording...";
                break;
            case AppState::Transcribing:
                icon = g_icon_transcribing;
                fallbackEmoji = @"⏳";
                statusText = @"Transcribing...";
                break;
            case AppState::Error:
                icon = g_icon_error;
                fallbackEmoji = @"⚠️";
                statusText = @"Error";
                break;
        }

        // Use SF Symbol image if available, otherwise fallback to emoji
        if (icon) {
            button.image = icon;
            button.title = @"";
        } else {
            button.image = nil;
            button.title = fallbackEmoji;
        }

        NSMenuItem *menuStatusItem = [statusItem.menu itemWithTag:100];
        if (menuStatusItem) {
            menuStatusItem.title = statusText;
        }
    });
}

} // namespace whispr

extern "C" void run_macos_event_loop() {
    @autoreleasepool {
        NSApplication *app = [NSApplication sharedApplication];

        // Use Accessory policy - no dock icon, just menu bar
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];

        g_delegate = [[VoxTypeAppDelegate alloc] init];
        app.delegate = g_delegate;

        // Activate the app
        [app activateIgnoringOtherApps:YES];

        NSLog(@"Starting NSApplication run loop");
        [app run];
    }
}
