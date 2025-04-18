// MIT License
// Copyright (c) 2025 dbjwhs

#include "../../include/cql/project_utils.hpp"

// Check if compiling on macOS
#if defined(__APPLE__) && defined(__MACH__)

// Objective-C++ implementation for macOS clipboard operations
#include <Cocoa/Cocoa.h>

namespace clipboard {

bool copy_to_clipboard(const std::string& text) {
    @autoreleasepool {
        // Create NSString from C++ string
        NSString* nsString = [NSString stringWithUTF8String:text.c_str()];
        
        // Get the pasteboard
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        [pasteboard clearContents];
        
        // Write string to pasteboard
        BOOL success = [pasteboard writeObjects:@[nsString]];
        return success == YES;
    }
}

std::string get_from_clipboard() {
    @autoreleasepool {
        // Get the pasteboard
        NSPasteboard* pasteboard = [NSPasteboard generalPasteboard];
        
        // Read the string
        NSString* nsString = [pasteboard stringForType:NSPasteboardTypeString];
        
        // Convert to C++ string
        if (nsString) {
            return {[nsString UTF8String]};
        } else {
            return "";
        }
    }
}

} // namespace clipboard

#endif // defined(__APPLE__) && defined(__MACH__)
