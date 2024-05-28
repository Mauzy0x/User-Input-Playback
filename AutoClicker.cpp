/*
    This is an auto clicker program designed to record the user's mouse and keyboard actions then play it back.
    Each new event will be stored in an enumerable struct. The user will select the Sleep duration between each played back
    keystroke and mouse click.

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


// Define a structure for storing either a mouse click or a keyboard event
struct Event {
    enum EventType { MouseClick, KeyPress } type;
    union {
        struct { int x, y; } mouseClick;
        struct { DWORD vkCode; bool isKeyDown; } keyPress;
    };
};

/// Function Declarations
LRESULT CALLBACK LowLevelMouseProc(int, WPARAM, LPARAM);
LRESULT CALLBACK LowLevelKeyboardProc(int, WPARAM, LPARAM);

void SetCapsLock(BOOL state);
void playback_recording(u_int&, u_int&);
void reset_keyboard_state();
void reset_mouse_state();
void display_banner();
u_int welcome_message();
void user_time_tuning(u_int&, u_int&);

/// Global variables
// Hook handles
HHOOK mouse_hook;
HHOOK keyboard_hook;

bool stop_message_loop = false;   // Flag to stop recording and listening for events
std::vector<Event> events;


int main() {

    display_banner();
    u_int iterations = welcome_message();                                                                                                         
                                                                                                        
    /// Set hooks
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

    Sleep(100);

    std::cout << "\n Listening for mouse clicks and key presses..." << std::endl;

    // Read messages from the Hook until no longer recieving a signal or the user cancles recording
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
    
    
    u_int time_between_clicks = 1000;
    u_int time_between_keystroke = 500;
    user_time_tuning(time_between_clicks, time_between_keystroke);

    std::cout << "\nPlaying back";

    // Perform recorded operations for given number of iterations
    for(u_int i = 0; i < iterations; i++) {
        playback_recording(time_between_clicks, time_between_keystroke);
        reset_keyboard_state();
        reset_mouse_state();
        std::cout << ".";
    }

    return EXIT_SUCCESS;
} // end main


/// Function Definitions --------------------------------------------------

/// LowLevelMouseProc function Description:
/// Mouse hook callback function
LRESULT CALLBACK LowLevelMouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT* mouseStruct = (MSLLHOOKSTRUCT*)lParam;
        if (mouseStruct != nullptr) {
            // Check for left mouse button down event
            if (wParam == WM_LBUTTONDOWN) {
                int x = mouseStruct->pt.x;
                int y = mouseStruct->pt.y;

                // Create an Event struct and push it into the vector
                Event event;
                event.type = Event::MouseClick;
                event.mouseClick.x = x;
                event.mouseClick.y = y;
                events.push_back(event);

                // Print the coordinates of the click
                std::cout << "Mouse clicked at (" << x << ", " << y << ")" << std::endl;
            }
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(mouse_hook, nCode, wParam, lParam);
} // end LowLevelMouseProc



/// LowLevelKeyboardProc function Description:
/// Keyboard hook callback function
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

            // Create an Event struct and push it into the vector
            Event event;
            event.type = Event::KeyPress;
            event.keyPress.vkCode = kbStruct->vkCode;
            event.keyPress.isKeyDown = isKeyDown;
            events.push_back(event);

            // Print the key event
            std::cout << "Key " << (isKeyDown ? "pressed" : "released") << ": " << kbStruct->vkCode << std::endl;
        }
    }

    // Call the next hook in the chain
    return CallNextHookEx(keyboard_hook, nCode, wParam, lParam);
} // end LowLevelKeyboardProc


/// playback_recording function Description:
/// Enumerate over the events and replay each mouse and keyboard event in the order they were recorded.
void playback_recording(u_int &time_between_clicks, u_int &time_between_keystrokes) {

    for (const auto& event : events) {
        if (event.type == Event::MouseClick) {

            SetCursorPos(event.mouseClick.x, event.mouseClick.y);
            //std::cout << "Moved mouse to (" << event.mouseClick.x << ", " << event.mouseClick.y << ")" << std::endl;
            // Simulate a left mouse button click
            mouse_event(MOUSEEVENTF_LEFTDOWN, event.mouseClick.x, event.mouseClick.y, 0, 0);
            mouse_event(MOUSEEVENTF_LEFTUP, event.mouseClick.x, event.mouseClick.y, 0, 0);
            Sleep(time_between_clicks);
        } else if (event.type == Event::KeyPress) {

            // Simulate key press and release
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = event.keyPress.vkCode;
            input.ki.dwFlags = event.keyPress.isKeyDown ? 0 : KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
            //std::cout << "Key " << (event.keyPress.isKeyDown ? "pressed" : "released") << ": " << event.keyPress.vkCode << std::endl;
            Sleep(time_between_keystrokes);
        }
    }
} // end playback_recording


/// reset_keyboard_state function Description:
/// Reset the keyboard state by releasing all keys
void reset_keyboard_state() {
    for (int i = 0; i < 256; i++) {
        if (GetAsyncKeyState(i) & 0x8000) {
            INPUT input = {0};
            input.type = INPUT_KEYBOARD;
            input.ki.wVk = i;
            input.ki.dwFlags = KEYEVENTF_KEYUP;
            SendInput(1, &input, sizeof(INPUT));
        }
    }
} // end resetKeyboardState

/// reset_mouse_state function Description:
/// Reset the mouse state by lifting all of the buttons
void reset_mouse_state() {
    // Simulate mouse button release for left, right, and middle buttons
    mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, 0);
    mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, 0);
}

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


void display_banner() {
    std::cout << R"( 
     _____ ______   ________  ___  ___  ________      ___    ___ ________     ___    ___ ________  ________
    |\   _ \  _   \|\   __  \|\  \|\  \|\_____  \    |\  \  /  /|\   __  \   |\  \  /  /|\   __  \|\   __  \
    \ \  \\\__\ \  \ \  \|\  \ \  \\\  \\|___/  /|   \ \  \/  / | \  \|\  \  \ \  \/  / | \  \|\  \ \  \|\  \
     \ \  \\|__| \  \ \   __  \ \  \\\  \   /  / /    \ \    / / \ \  \\\  \  \ \    / / \ \  \\\  \ \  \\\  \
      \ \  \    \ \  \ \  \ \  \ \  \\\  \ /  /_/__    \/  /  /   \ \  \\\  \  /     \/   \ \  \\\  \ \  \\\  \
       \ \__\    \ \__\ \__\ \__\ \_______\\________\__/  / /      \ \_______\/  /\   \    \ \_______\ \_______\
        \|__|     \|__|\|__|\|__|\|_______|\|_______|\___/ /        \|_______/__/ /\ __\    \|_______|\|_______|
                                                    \|___|/                  |__|/ \|__|                                   
                                                                                                       
                                                                                                       
     ___  ________   ________  ___  ___  _________        ________  _______   ________  ___       ________      ___    ___
    |\  \|\   ___  \|\   __  \|\  \|\  \|\___   ___\     |\   __  \|\  ___ \ |\   __  \|\  \     |\   __  \    |\  \  /  /|
    \ \  \ \  \\ \  \ \  \|\  \ \  \\\  \|___ \  \_|     \ \  \|\  \ \   __/|\ \  \|\  \ \  \    \ \  \|\  \   \ \  \/  / /
     \ \  \ \  \\ \  \ \   ____\ \  \\\  \   \ \  \       \ \   _  _\ \  \_|/_\ \   ____\ \  \    \ \   __  \   \ \    / /
      \ \  \ \  \\ \  \ \  \___|\ \  \\\  \   \ \  \       \ \  \\  \\ \  \_|\ \ \  \___|\ \  \____\ \  \ \  \   \/  /  /
       \ \__\ \__\\ \__\ \__\    \ \_______\   \ \__\       \ \__\\ _\\ \_______\ \__\    \ \_______\ \__\ \__\__/  / /
        \|__|\|__| \|__|\|__|     \|_______|    \|__|        \|__|\|__|\|_______|\|__|     \|_______|\|__|\|__|\___/ /)" << std::endl;
}

u_int welcome_message() {
    std::cout << "Welcome to Mauzy's playback program! \n" <<
                 "Whenever you are ready, the program will record your left mouse clicks as well as keyboard events.\n\n" <<
                 "Press CTRL + ALT + Q at the same time to stop recording.";
    
    for (int i = 0; i < 4; i++) {
        Sleep(1000);
        std::cout << ".";
    }
    std::cout << std::endl << std::endl;

    // Make sure the user is entering a valid integer
    u_int iterations = 0;
    u_int attempts = 0;
    u_int max_attempts = 3;
    while (attempts < max_attempts) {
        std::cout << "Before starting, how many times would you like to play back your movements?\n"
                << "Please enter an integer: ";
        std::cin >> iterations;
        
        if (std::cin.fail() || iterations < 1) {
            
            std::cin.clear(); std::cin.ignore();

            attempts++;

            std::cout << "Invalid input. Please enter a positive integer." << std::endl;
            std::cout << "Attempt " << attempts << " of " << max_attempts << ".\n\n";
        } else {
            break;
        }
    }

    if (attempts == max_attempts) {
        std::cout << "Too many invalid attempts. Exiting program." << std::endl;
        exit(EXIT_FAILURE); // Exit the program with a failure code
    }

    return iterations;
} // end welcome_message


void user_time_tuning(u_int &time_between_clicks, u_int &time_between_keystrokes) {
    
    u_int attempts = 0;
    const u_int max_attempts = 3;

    std::cout << "Great! Now that we have that recorded you should fine-tune the time between each keystroke and mouse click.\n"
              << "You should keep in mind there can be loading time between each click. If you don't have to worry about loading time then keep the time short.\n";
    std::cout << "\nDefaults:\n"
              << "Time between clicks = 1000ms (1 second)\n"
              << "Time between keystrokes = 500ms (0.5 second)\n\n";

    // Get time between clicks
    while (attempts < max_attempts) {
        std::cout << "Please enter time between clicks in ms (example `1000`): ";
        std::cin >> time_between_clicks;

        if (std::cin.fail() || time_between_clicks < 1) {
            std::cin.clear(); // Clear the error flag set by std::cin.fail()
            std::cin.ignore(); // Ignore the invalid input
            attempts++;
            std::cout << "Invalid input. Please enter a positive integer." << std::endl;
            std::cout << "Attempt " << attempts << " of " << max_attempts << "." << std::endl;
        } else {
            break;
        }
    }

    if (attempts == max_attempts) {
        std::cout << "Too many invalid attempts. Exiting program." << std::endl;
        exit(EXIT_FAILURE); // Exit the program with a failure code
    }

    // Reset attempts for the next input
    attempts = 0;

    // Get time between keystrokes
    while (attempts < max_attempts) {
        std::cout << "Please enter time between keystrokes in ms (example `500`): ";
        std::cin >> time_between_keystrokes;

        if (std::cin.fail() || time_between_keystrokes < 1) {
            std::cin.clear(); // Clear the error flag set by std::cin.fail()
            std::cin.ignore(); // Ignore the invalid input
            attempts++;
            std::cout << "Invalid input. Please enter a positive integer." << std::endl;
            std::cout << "Attempt " << attempts << " of " << max_attempts << "." << std::endl;
        } else {
            break;
        }
    }

    if (attempts == max_attempts) {
        std::cout << "Too many invalid attempts. Exiting program." << std::endl;
        exit(EXIT_FAILURE); // Exit the program with a failure code
    }
}



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