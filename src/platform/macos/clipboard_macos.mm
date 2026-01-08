#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#include "clipboard.hpp"
#include <iostream>

namespace whispr {

bool Clipboard::set_text(const std::string& text) {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];

        NSString* nsText = [NSString stringWithUTF8String:text.c_str()];
        if (!nsText) {
            std::cerr << "Failed to create NSString from text" << std::endl;
            return false;
        }

        BOOL success = [pasteboard setString:nsText forType:NSPasteboardTypeString];
        return success == YES;
    }
}

std::string Clipboard::get_text() {
    @autoreleasepool {
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        NSString* text = [pasteboard stringForType:NSPasteboardTypeString];

        if (text) {
            return std::string([text UTF8String]);
        }
        return "";
    }
}

bool Clipboard::paste() {
    @autoreleasepool {
        // Simulate Cmd+V
        CGEventSourceRef source = CGEventSourceCreate(kCGEventSourceStateCombinedSessionState);
        if (!source) {
            std::cerr << "Failed to create event source" << std::endl;
            return false;
        }

        // Create key down event for 'v' with Command modifier
        CGEventRef keyDown = CGEventCreateKeyboardEvent(source, kVK_ANSI_V, true);
        CGEventRef keyUp = CGEventCreateKeyboardEvent(source, kVK_ANSI_V, false);

        if (!keyDown || !keyUp) {
            if (keyDown) CFRelease(keyDown);
            if (keyUp) CFRelease(keyUp);
            CFRelease(source);
            return false;
        }

        // Set Command modifier
        CGEventSetFlags(keyDown, kCGEventFlagMaskCommand);
        CGEventSetFlags(keyUp, kCGEventFlagMaskCommand);

        // Post events
        CGEventPost(kCGHIDEventTap, keyDown);
        CGEventPost(kCGHIDEventTap, keyUp);

        CFRelease(keyDown);
        CFRelease(keyUp);
        CFRelease(source);

        return true;
    }
}

} // namespace whispr
