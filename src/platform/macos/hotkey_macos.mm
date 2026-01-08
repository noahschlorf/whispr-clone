#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <CoreGraphics/CoreGraphics.h>
#include "hotkey_manager.hpp"
#include <iostream>

namespace whispr {

// Global state for the event tap
static CGEventRef event_callback(CGEventTapProxy proxy, CGEventType type,
                                  CGEventRef event, void* user_info);

struct MacOSHotkeyState {
    CFMachPortRef event_tap = nullptr;
    CFRunLoopSourceRef run_loop_source = nullptr;
    CFRunLoopRef run_loop = nullptr;
    HotkeyManager* manager = nullptr;
    uint32_t target_keycode = 0;
    bool key_pressed = false;
};

static MacOSHotkeyState* g_state = nullptr;

bool HotkeyManager::initialize() {
    if (platform_handle_) return true;

    g_state = new MacOSHotkeyState();
    g_state->manager = this;
    platform_handle_ = g_state;

    return true;
}

void HotkeyManager::shutdown() {
    stop();

    if (g_state) {
        delete g_state;
        g_state = nullptr;
    }
    platform_handle_ = nullptr;
}

bool HotkeyManager::start() {
    if (running_.load()) return true;
    if (!g_state) return false;

    g_state->target_keycode = keycode_;
    g_state->key_pressed = false;

    // Create event tap for key events
    CGEventMask event_mask = (1 << kCGEventKeyDown) | (1 << kCGEventKeyUp) |
                             (1 << kCGEventFlagsChanged);

    g_state->event_tap = CGEventTapCreate(
        kCGSessionEventTap,
        kCGHeadInsertEventTap,
        kCGEventTapOptionDefault,
        event_mask,
        event_callback,
        g_state
    );

    if (!g_state->event_tap) {
        std::cerr << "Failed to create event tap. Make sure accessibility permissions are granted." << std::endl;
        std::cerr << "Go to: System Preferences > Security & Privacy > Privacy > Accessibility" << std::endl;
        return false;
    }

    g_state->run_loop_source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault,
                                                             g_state->event_tap, 0);

    running_.store(true);

    // Start listener thread
    listener_thread_ = std::thread([this]() {
        g_state->run_loop = CFRunLoopGetCurrent();
        CFRunLoopAddSource(g_state->run_loop, g_state->run_loop_source,
                          kCFRunLoopCommonModes);
        CGEventTapEnable(g_state->event_tap, true);

        CFRunLoopRun();
    });

    return true;
}

void HotkeyManager::stop() {
    if (!running_.load()) return;

    running_.store(false);

    if (g_state) {
        if (g_state->event_tap) {
            CGEventTapEnable(g_state->event_tap, false);
        }

        if (g_state->run_loop) {
            CFRunLoopStop(g_state->run_loop);
        }

        if (g_state->run_loop_source) {
            CFRelease(g_state->run_loop_source);
            g_state->run_loop_source = nullptr;
        }

        if (g_state->event_tap) {
            CFRelease(g_state->event_tap);
            g_state->event_tap = nullptr;
        }
    }

    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

static CGEventRef event_callback(CGEventTapProxy proxy, CGEventType type,
                                  CGEventRef event, void* user_info) {
    (void)proxy;

    auto* state = static_cast<MacOSHotkeyState*>(user_info);
    if (!state || !state->manager) return event;

    // Handle tap disabled (system can disable it under load)
    if (type == kCGEventTapDisabledByTimeout || type == kCGEventTapDisabledByUserInput) {
        CGEventTapEnable(state->event_tap, true);
        return event;
    }

    CGKeyCode keycode = static_cast<CGKeyCode>(CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));

    // Check for our target key
    // For modifier keys (like Option), we use kCGEventFlagsChanged
    if (type == kCGEventFlagsChanged) {
        CGEventFlags flags = CGEventGetFlags(event);

        // Check for Right Option key (keycode 61)
        bool right_option_pressed = (flags & kCGEventFlagMaskAlternate) &&
                                    (keycode == 61 || keycode == state->target_keycode);

        if (right_option_pressed && !state->key_pressed) {
            state->key_pressed = true;
            if (state->manager->callback_) {
                state->manager->callback_(true);
            }
        } else if (!right_option_pressed && state->key_pressed && keycode == state->target_keycode) {
            state->key_pressed = false;
            if (state->manager->callback_) {
                state->manager->callback_(false);
            }
        }
    }

    return event;
}

} // namespace whispr
