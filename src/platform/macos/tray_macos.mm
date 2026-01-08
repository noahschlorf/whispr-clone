#import <Cocoa/Cocoa.h>
#include "app.hpp"
#include <iostream>

static whispr::App* g_app = nil;
static NSStatusItem* g_status_item = nil;
static BOOL g_status_ready = NO;

// Cache SF Symbol images for performance
static NSImage* g_icon_idle = nil;
static NSImage* g_icon_recording = nil;
static NSImage* g_icon_transcribing = nil;
static NSImage* g_icon_error = nil;

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

// Initialize all icons (called once on app start)
static void initializeIcons() {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        g_icon_idle = loadSFSymbol(@"mic.fill");
        g_icon_recording = loadSFSymbol(@"mic.circle.fill");
        g_icon_transcribing = loadSFSymbol(@"waveform");
        g_icon_error = loadSFSymbol(@"exclamationmark.triangle.fill");

        // Retain icons so they persist
        if (g_icon_idle) [g_icon_idle retain];
        if (g_icon_recording) [g_icon_recording retain];
        if (g_icon_transcribing) [g_icon_transcribing retain];
        if (g_icon_error) [g_icon_error retain];

        NSLog(@"Icons initialized - idle:%@ recording:%@ transcribing:%@ error:%@",
              g_icon_idle ? @"OK" : @"FAIL",
              g_icon_recording ? @"OK" : @"FAIL",
              g_icon_transcribing ? @"OK" : @"FAIL",
              g_icon_error ? @"OK" : @"FAIL");
    });
}

// Track current quality mode
static whispr::ModelQuality g_current_quality = whispr::ModelQuality::Balanced;

@interface WhisprAppDelegate : NSObject <NSApplicationDelegate>
- (void)updateQualityMenu;
@end

@implementation WhisprAppDelegate

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

            NSMenu *menu = [[NSMenu alloc] init];

            // Status item
            NSMenuItem *statusItem = [[NSMenuItem alloc] initWithTitle:@"Whispr - Ready" action:nil keyEquivalent:@""];
            statusItem.enabled = NO;
            statusItem.tag = 100;
            [menu addItem:statusItem];

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
            NSMenuItem *quitItem = [[NSMenuItem alloc] initWithTitle:@"Quit Whispr" action:@selector(quitApp:) keyEquivalent:@"q"];
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

static WhisprAppDelegate* g_delegate = nil;

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

        NSImage* icon = nil;
        NSString* fallbackEmoji = @"🎤";
        NSString* statusText = @"Whispr - Ready";

        switch (captured_state) {
            case AppState::Idle:
                icon = g_icon_idle;
                fallbackEmoji = @"🎤";
                statusText = @"Whispr - Ready";
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

        g_delegate = [[WhisprAppDelegate alloc] init];
        app.delegate = g_delegate;

        // Activate the app
        [app activateIgnoringOtherApps:YES];

        NSLog(@"Starting NSApplication run loop");
        [app run];
    }
}
