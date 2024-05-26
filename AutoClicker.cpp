/*
    This is an auto clicker program designed to record the user's movements then stop recording.

    Keybind to stop recording: CTRL + ALT + Q
    
    Author: Mauzy0x00
    Date:   5/25/2024


    References: 
                -- WinAPI
                    - Hooks: https://learn.microsoft.com/en-us/windows/win32/winmsg/about-hooks
*/

#include <iostream>
#include <Windows.h>
#include <string>
#include <vector>


/// Structures
struct ClickedCoordinates {
    int x;
    int y;
};

struct KeyEvent {
    DWORD vkCode; 
    bool isKeyDown;
};


/// Function Definitions
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);

void stall_Program(int time);
void SetCapsLock(BOOL state);
void send_Key(char key, double duration);
void playback_recording();


/// Global variables

// Hook handles
HHOOK mouse_hook;
HHOOK keyboard_hook;

bool stop_message_loop = false;   // Flag to stop recording and listening for events
std::vector<ClickedCoordinates> coordinate_list;
std::vector<KeyEvent> key_events;

int main() {
    
    ClickedCoordinates coord;   
 
    // Set the mouse hook
    mouse_hook = SetWindowsHookEx(WH_MOUSE_LL, LowLevelMouseProc, nullptr, 0);
    if (mouse_hook == nullptr) {
        std::cerr << "Failed to set mouse hook" << std::endl;
        return 1;
    }   
    
    // Set the keyboard hook
    keyboard_hook = SetWindowsHookEx(WH_KEYBOARD_LL, LowLevelKeyboardProc, nullptr, 0);
    if (keyboard_hook == nullptr) {
        std::cerr << "Failed to set keyboard hook" << std::endl;
        return 1;
    }

    std::cout << "Listening for mouse clicks and key presses..." << std::endl;

    // Read messages from the Hook until no longer recieving a signal 
    MSG msg;
    while (!stop_message_loop) {
        // Use PeekMessage instead of GetMessage because GetMessage is blocking and PeekMessage is not
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        Sleep(10);
    }

    // Unhook the hooks because we are not using them anymore
    UnhookWindowsHookEx(mouse_hook);
    UnhookWindowsHookEx(keyboard_hook);

    std::cout << "Completed recording.\n";


    // Perform recorded operations for given number of iterations
    playback_recording();

    stall_Program(5);

    return EXIT_SUCCESS;
} // end main


/// LowLevelMouseProc function Description:
/// Mouse hook callback function
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT *mouseStruct = (MSLLHOOKSTRUCT *)lParam;
        if (mouseStruct != nullptr) {
            // Check for left mouse button down event
            if (wParam == WM_LBUTTONDOWN) {
                int x = mouseStruct->pt.x;
                int y = mouseStruct->pt.y;

                // Create a Coordinate struct and push it into the vector
                ClickedCoordinates coord;
                coord.x = x;
                coord.y = y;
                coordinate_list.push_back(coord);

                // Print the coordinates of the click
                std::cout << "Mouse clicked at (" << x << ", " << y << ")" << std::endl;
            }
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(mouse_hook, nCode, wParam, lParam);
} // end LowLevelMouseProc


/// LowLevelKeyboardProc function Description:
///
LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        KBDLLHOOKSTRUCT* kbStruct = (KBDLLHOOKSTRUCT*)lParam;
        if (kbStruct != nullptr) {
            bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);

            // Check for Ctrl+Alt+Q combination to stop the message loop
            if (isKeyDown && (GetAsyncKeyState(VK_CONTROL) & 0x8000) && (GetAsyncKeyState(VK_MENU) & 0x8000) && kbStruct->vkCode == 'Q') {
                stop_message_loop = true;
                return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
            }

            // Create a KeyEvent struct and push it into the vector
            KeyEvent keyEvent;
            keyEvent.vkCode = kbStruct->vkCode;
            keyEvent.isKeyDown = isKeyDown;
            key_events.push_back(keyEvent);
            std::cout << stop_message_loop << std::endl;
            // Print the key event
            std::cout << "Key " << (keyEvent.isKeyDown ? "pressed" : "released") << ": " << keyEvent.vkCode << std::endl;
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
}
// end LowLevelKeyboardProc

/// @brief 
void playback_recording() {
    
    // Print coordinates stored in the vector
    std::cout << "Stored Coordinates:" << std::endl;
    for (const auto& coord : coordinate_list) {
        std::cout << "(" << coord.x << ", " << coord.y << ")" << std::endl;
    }
    
    // Print all stored key events
    std::cout << "Stored Key Events:" << std::endl;
    for (const auto& keyEvent : key_events) {
        std::cout << "Key " << (keyEvent.isKeyDown ? "pressed" : "released") << ": " << keyEvent.vkCode << std::endl;
    }



} // end playback_recording

/// Take the key to be pressed from convert_Note function and send the key to the focused program
void send_Key(char key, double duration) {
    // Set up the input event for a key press
    INPUT input = {};
    input.type = INPUT_KEYBOARD;


    input.ki.wVk = key;     // virtual-key code for the key needed
    input.ki.dwFlags = 0;   // key press

    // Send the key press event
    SendInput(1, &input, sizeof(INPUT));

    // Note duration
    Sleep(duration);

    // Set up the input event for the "s" key release
    input.ki.dwFlags = KEYEVENTF_KEYUP; // key release

    // Send the key release event
    SendInput(1, &input, sizeof(INPUT));
} // end of send_Key


/// SetCapsLock function Description:
/// Toggle the caps lock on the computer. This is to enable key capture within Waveform
/// Got this code from Microsoft documentation and edited for VK_CAPITAL URL:https://support.microsoft.com/en-us/topic/howto-toggle-the-num-lock-caps-lock-and-scroll-lock-keys-1718b9bd-5ebf-f3ab-c249-d5312c93d2d7
void SetCapsLock(BOOL state) {
    BYTE keyState[256]; // what does this do? -- Edit: declares an array of 256 bytes which is used to store the current state of all keys on the keyboard - neat

    GetKeyboardState((LPBYTE)&keyState);
    if( (state && !(keyState[VK_CAPITAL] & 1)) ||   
        (!state && (keyState[VK_CAPITAL] & 1)) )        // this is a funky looking if statement. kinda cool!
    {
    // Simulate a key press
        keybd_event( VK_CAPITAL,
                    0x45,
                    KEYEVENTF_EXTENDEDKEY | 0,
                    0 );

    // Simulate a key release
        keybd_event( VK_CAPITAL,
                    0x45,
                    KEYEVENTF_EXTENDEDKEY | KEYEVENTF_KEYUP,
                    0);
    }
} // end SetCapsLock

// stall_Program Description: 
// Have the program wait [time](s) until the program beings running in Waveform
void stall_Program(int time){
    std::cout << "Hey there!~\n";
    std::cout << "Starting Count down...\n";

    for(int i = time; i > 0; i--){
        std::cout << "T-" << i << std::endl;
        Sleep(1000); // 1 second
    }
} // end stall_Program


/*
#Notes:

##Windows API:

    Low Level Mouse Proc https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelmouseproc
    Global hook procedures must be placed in a DLL separate from the application installing the hook procedure.
    The installing application must have the handle to the DLL module before it can install the hook procedure. 
    To retrieve a handle to the DLL module, call the `LoadLibrary` function with the name of the DLL. After you have obtained the handle, you can call the `GetProcAddress` function 
    to retrieve a pointer to the hook procedure. 
    Finally, use `SetWindowsHookEx` to install the hook procedure address in the appropriate hook chain. 
    SetWindowsHookEx passes the module handle, a pointer to the hook-procedure entry point, and 0 for the thread identifier, indicating that the hook procedure should be associated with all threads in the same desktop as the calling thread.


    The INPUT structure contains a union called ki that is used specifically for describing keyboard input events. ki stands for "keyboard input".
    Within the ki union, there are several members that can be used to describe a keyboard input event, including wVk, wScan, and dwFlags. wVk is the virtual-key code for the key being pressed, and wScan is the hardware scan code for the key. 
    dwFlags is a set of flags that describe the keyboard input event, such as whether the key is being pressed or released, or whether it is being repeated.

    In the example I provided earlier, we set input.ki.dwFlags to 0 to indicate that the "s" key is being pressed. Later, we set input.ki.dwFlags to KEYEVENTF_KEYUP to indicate that the "s" key is being released.

    The dw in dwFlags stands for "double word", which is a data type that is 32 bits (4 bytes) in size. The Flags part of dwFlags indicates that it is a set of bit flags, where each bit represents a specific value or option.
    By setting different combinations of bits in dwFlags, we can specify different options for the keyboard input event.
*/