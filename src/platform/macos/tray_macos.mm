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

@interface WhisprAppDelegate : NSObject <NSApplicationDelegate>
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

            NSMenuItem *statusItem = [[NSMenuItem alloc] initWithTitle:@"Whispr - Ready" action:nil keyEquivalent:@""];
            statusItem.enabled = NO;
            statusItem.tag = 100;
            [menu addItem:statusItem];

            [menu addItem:[NSMenuItem separatorItem]];

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
    if (!g_status_ready || !g_status_item) return;

    dispatch_async(dispatch_get_main_queue(), ^{
        if (!g_status_ready || !g_status_item) return;

        NSImage* icon = nil;
        NSString* fallbackEmoji = @"🎤";
        NSString* status = @"Whispr - Ready";

        switch (state) {
            case AppState::Idle:
                icon = g_icon_idle;
                fallbackEmoji = @"🎤";
                status = @"Whispr - Ready";
                break;
            case AppState::Recording:
                icon = g_icon_recording;
                fallbackEmoji = @"🔴";
                status = @"Recording...";
                break;
            case AppState::Transcribing:
                icon = g_icon_transcribing;
                fallbackEmoji = @"⏳";
                status = @"Transcribing...";
                break;
            case AppState::Error:
                icon = g_icon_error;
                fallbackEmoji = @"⚠️";
                status = @"Error";
                break;
        }

        // Use SF Symbol image if available, otherwise fallback to emoji
        if (icon) {
            g_status_item.button.image = icon;
            g_status_item.button.title = @"";
        } else {
            g_status_item.button.image = nil;
            g_status_item.button.title = fallbackEmoji;
        }

        NSMenuItem *statusItem = [g_status_item.menu itemWithTag:100];
        if (statusItem) {
            statusItem.title = status;
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
