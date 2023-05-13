#pragma once
#include "../defines.h"
#include "engine_math.h"
#include "timer.h"
#include "win32_lib.h"
#include <Windows.h>
#include "input_buttons.h"
#include <glm/glm.hpp>

#define INPUT_DEBUG
#ifdef INPUT_DEBUG
#include <imgui.h>
#endif

typedef i32 WheelDelta;

struct MouseDelta 
{
    i32 dx;
    i32 dy;
};

#define BUTTON_DOWN 1
#define BUTTON_HELD 2
#define BUTTON_DBLCLICK 4

struct ButtonState 
{
    // bit 0: up/down
    // bit 1: held
    // bit 2: double_clicked
    u8 state; 
};

inline b8 button_down(ButtonState btn) { return (btn.state & BUTTON_DOWN) != 0; }
inline b8 button_held(ButtonState btn) { return (btn.state & BUTTON_HELD) != 0; }
inline b8 button_dblclick(ButtonState btn) { return (btn.state & BUTTON_DBLCLICK) != 0; }

#define BUTTON_PARTS(Button) button_down(Button), button_held(Button), button_dblclick(Button)

struct Keyboard 
{
    ButtonState keys[256];
    // modifier keys
    ButtonState anyshift, tab_down, backspace, enter, space_down, anyctrl, capslock;
};

struct Mouse
{
    ButtonState left;
    ButtonState right;
    ButtonState middle;
    ButtonState x1;
    ButtonState x2;
    glm::vec3 cursor; // X = clip x, Y = clip y, Z = scroll wheel
    glm::vec3 cursor_delta; // the change in the cursor since last frame - might also be computable
    glm::vec3 cursor_drag_delta; // distance between the cursor when it started dragging and now
    glm::vec3 raw_input_cursor_delta; // if we are using raw_input, then this holds the value of the most-recent rawinput reading  
};


enum EventType 
{
    MouseMove,
    MouseLeftClick,
    MouseLeftDblClick,
    MouseLeftRelease,
    MouseRightClick,
    MouseRightDblClick,
    MouseRightRelease,
    MouseMiddleClick,
    MouseMiddleDblClick,
    MouseMiddleRelease,
    MouseExtraClick1,
    MouseExtraDblClick1,
    MouseExtraRelease1,
    MouseExtraClick2,
    MouseExtraDblClick2,
    MouseExtraRelease2,
    DragStart,
    Dragging,
    DragEnd,
    MouseScroll,
    KeyDown,
    KeyUp,
    KeyHeld,
    KeyCombo,
    KeyChord
};

// maybe this would be a good place for a union or actually using inheritance?
struct InputEvent 
{
    EventType type;
    u64 timestamp;
    Mouse mouse_state;
    Keyboard kb_state;
};

enum InputFocusScope 
{
    IMGUI,
    WORLD_EDIT,
    CAMERA_CTRL,
    SCOPE_ALL
};

// TODO input layers might be a natural way to let listeners declare when they
// are interested in certain messages. Maybe an array of layers like
// register_event_listener( {Layer_Global, Layer_EditorImgui, Layer_Scene}, ..r)
// if a component is interested in multiple scopes, it can simply register 
// 2 seperate callbacks 
struct EventListener 
{
    void *self;
    void *function;
    InputFocusScope scope_filter;
};

struct Win32RawInputScratchMemory
{
    u32 packet_count;
    u32 rawinput_packet_size;
    void *rawinput_packets;
};

struct InputEventCallbacks
{
    u32 max_subscribers;
    u32 subscribers_count;
    EventListener *subscribers;
};

struct InputEventQueue
{
    u32 max_events;
    u32 event_count;
    InputEvent *events;
};

struct InputSystem 
{
    Timer timer;
    f32 deltatime_for_frame;
    f32 last_frame_time;
    f32 last_input_time;
    Mouse mouse{};
    Keyboard keyboard{};
    bool viewport_focused = 0; // whether the imgui viewport for the 3d scene is focused
    bool imgui_active = 0;   // if any imgui window is being used
    bool win32_window_active = 0; // if the win32 window is active 
    bool showing_cursor;

    f32 remember_x, remember_y; // used to restore the mouse position where it was before we hid it
    bool lock_cursor;
                                  
    InputFocusScope focus_scope;

    InputEventCallbacks input_callbacks;
    InputEventQueue events;

    b32 using_raw_input;
    Win32RawInputScratchMemory raw_input;
};

InputSystem input_create(HWND hwnd, b8 enable_raw_input);
void input_destroy(InputSystem* input);

void input_process_rawinput_packet(InputSystem* input, RAWINPUT *const rawinput_packet);

struct Win32WndProcData 
{ 
    HWND window;
    u32 message;
    LPARAM lparam;
    WPARAM wparam;
};

LRESULT input_process_win32_message(InputSystem *input, Win32WndProcData win32_data);

/* NOTE:
   Win32 will "buffer" LOTS of rawinput packets when working with 
   high polling rate mice. This causes the WndProc/message queue to get *slammed* 
   with a huge amount of individual WM_INPUT messages. 
   To prevent this, at the beginning of each frame, call `input_process_win32_pending_rawinput_stream`
   which will process all the pending rawinput packets and clear up the windows message queue.
   Otherwise, it will lag like shit. */
void input_process_win32_pending_rawinput_stream(InputSystem* input);
void input_listen(InputSystem *input, EventListener listener);
void input_notify_listeners(InputSystem *input);
void input_push_event(InputSystem *input, InputEvent event);
void input_debug_ui(InputSystem *input);

// during start of frame, calculate the "held" buttons and associate timers to them
// clear one-shot values like mouse click flags
void input_begin_frame(InputSystem *input);
void input_end_frame(InputSystem *input);
glm::vec3 input_cursorpos(InputSystem *input);
glm::vec3 input_cursor_delta(InputSystem* input);
glm::vec3 input_cursor_drag_distance(InputSystem* input);
glm::vec3 input_rawinput_reading(InputSystem* input);
void input_hide_cursor(InputSystem* input);
void input_show_cursor(InputSystem* input);
void input_lock_cursor(InputSystem* input);
void input_unlock_cursor(InputSystem* input);

b8 input_down(InputSystem *input, u32 button);
b8 input_held(InputSystem *input, u32 button);
f32 input_mouse_wheel(InputSystem *input);
