#include "input_system.h"
#include "logging.h"
#include <errhandlingapi.h>
#include <memoryapi.h>
#include <time.h>
#include <windowsx.h>
#include <winuser.h>
#include <stdlib.h>
#define QWORD u64

InputSystem input_create(HWND hwnd, b8 enable_raw_input) 
{ 
    InputSystem input;

    input.deltatime_for_frame = 0.0;

    if(enable_raw_input)
    {
        RAWINPUTDEVICE devices[1]{};
        devices[0].hwndTarget = hwnd;
        devices[0].usUsagePage = 1;
        devices[0].usUsage = 2;
        devices[0].dwFlags = 0;

        if(RegisterRawInputDevices(devices, 1, sizeof(RAWINPUTDEVICE)))
        {
            LOG_INFO("registered raw devices input");
            input.raw_input.rawinput_packet_size = sizeof(RAWINPUT);
            input.raw_input.packet_count = 512;

            // https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-getrawinputbuffer
            // NOTE: requires that the memory buffer is aligned to a pointer boundary (hence the aligned_malloc + sizeof(void*))
            input.raw_input.rawinput_packets = ALIGNED_ALLOC(input.raw_input.packet_count * input.raw_input.rawinput_packet_size, sizeof(void*));
            Q_ASSERT(input.raw_input.rawinput_packets != 0); 
            if(input.raw_input.rawinput_packets)
            {
                input.using_raw_input = true;
            }
            else
            {
                input.using_raw_input = false;
                LOG_WARNING("Raw input was requested, but ALIGNED_ALLOC() failed to give us the memory!");
            }
        }
        else
        {
            input.using_raw_input = false;
            LOG_WARNING("Raw input was requested, but RegisterInputDevices failed!");
        }
    }
    else
    {
        LOG_INFO("InputSystem is not using raw input");
        input.using_raw_input = false;
    }

    input.focus_scope = WORLD_EDIT;
    input.input_callbacks.max_subscribers = 128; // even 128 would be a lot of callbacks!
    input.input_callbacks.subscribers_count = 0;
    input.input_callbacks.subscribers = new EventListener[input.input_callbacks.max_subscribers];

    input.events.event_count = 0;
    input.events.max_events = 512;
    input.events.events = new InputEvent[input.events.max_events];

    return (input);
}

void input_destroy(InputSystem* input)
{
    if (input->using_raw_input)
    {
        RAWINPUTDEVICE devices[1]{};
        devices[0].hwndTarget = NULL;
        devices[0].usUsagePage = 1;
        devices[0].usUsage = 2;
        devices[0].dwFlags = RIDEV_REMOVE;
        
        bool result = RegisterRawInputDevices(devices, 1, sizeof(RAWINPUTDEVICE));
        Q_ASSERT(result);
        LOG_INFO("unregistered raw devices input");
        if(input->raw_input.rawinput_packets)
        {
            ALIGNED_FREE(input->raw_input.rawinput_packets);
        }
    }
}

glm::vec3 input_extract_mouse_rawinput_reading(RAWINPUT *packet) 
{
    glm::vec3 rawinput_mouse_delta{};

    if (packet->header.dwType == RIM_TYPEMOUSE) 
    {
        // rawinput packets for mice are already in terms of a delta from the last reading.
        rawinput_mouse_delta.x = packet->data.mouse.lLastX;
        rawinput_mouse_delta.y = packet->data.mouse.lLastY;
        return rawinput_mouse_delta;
    }
    else
    {
        return glm::vec3{0.0, 0.0, 0.0};
    }
}

void input_process_win32_pending_rawinput_stream(InputSystem* input)
{
    u32 rawinput_structure_size = 0; // size of 1 rawinput packet, returned from GetRawInputBuffer
    u32 rawinput_buffer_size = 0;
    if (0 == GetRawInputBuffer(0, &rawinput_structure_size, sizeof(RAWINPUTHEADER))) 
    {
        // Check and make sure everything is matching up
        if(rawinput_structure_size == input->raw_input.rawinput_packet_size)
        {
            u32 our_buffer_size = input->raw_input.packet_count * input->raw_input.rawinput_packet_size;
            u32 packet_count = 0;
            glm::vec3 rawinput_delta_accum{0.0, 0.0, 0.0};
            while(( packet_count = GetRawInputBuffer((RAWINPUT *)input->raw_input.rawinput_packets, &our_buffer_size, sizeof(RAWINPUTHEADER)) ) > 0)
            {
                /* accumulate the delta for all the mouse messages in the queue */
                RAWINPUT *packet_current = (RAWINPUT*)input->raw_input.rawinput_packets;
                for(i32 i = 0; i < packet_count; ++i)
                {
                    glm::vec3 delta = input_extract_mouse_rawinput_reading(packet_current);
                    rawinput_delta_accum.x += delta.x;
                    rawinput_delta_accum.y += delta.y;
                    rawinput_delta_accum.z += delta.z;
                    packet_current = NEXTRAWINPUTBLOCK(packet_current);
                }
            }
            input->mouse.raw_input_cursor_delta.x += rawinput_delta_accum.x;
            input->mouse.raw_input_cursor_delta.y += rawinput_delta_accum.y;
            input->mouse.raw_input_cursor_delta.z += rawinput_delta_accum.z;
        }
    }
}

// handles the WM_XXX messages from wndproc
LRESULT input_process_win32_message(InputSystem* input, Win32WndProcData win32_data) 
{
    // LOG_INFO("handle win32 input");
    switch (win32_data.message) 
    {
        case WM_INPUT: {
            // handle raw input packet
            RAWINPUT raw_data{};
            u32 data_size_bytes{};

            if(GET_RAWINPUT_CODE_WPARAM(win32_data.wparam) == RIM_INPUT)
            {
                HRAWINPUT rawinput_handle = (HRAWINPUT)win32_data.lparam;

                if(0 == GetRawInputData(rawinput_handle, RID_INPUT, 0, &data_size_bytes, sizeof(RAWINPUTHEADER)))
                {
                    if(data_size_bytes == sizeof(RAWINPUT))
                    {
                        u32 bytes_copied = GetRawInputData(rawinput_handle, RID_INPUT, &raw_data, &data_size_bytes, sizeof(RAWINPUTHEADER));
                        if (bytes_copied == data_size_bytes) 
                        {
                            glm::vec3 raw_mouse_reading = input_extract_mouse_rawinput_reading(&raw_data);
                            input->mouse.raw_input_cursor_delta = raw_mouse_reading;
                        }
                        else
                        {
                            LOG_COM(GetLastError());
                            LOG_FATAL(""); // breakpoint for debugger
                            return DefRawInputProc(0, 0, sizeof(RAWINPUTHEADER));
                        }
                    }
                    else
                    {
                        LOG_WARNING("sizeof(RAWINPUT) != data_size_bytes({})", data_size_bytes);
                    }
                }
                else
                {
                    LOG_WARNING("GetRawInputData for structure size failed - returned non-zero");
                }
            }
            else
            {
                return DefRawInputProc(0, 0, sizeof(RAWINPUTHEADER));
            }

        } break;

        case WM_ACTIVATE: {
            if (LOWORD(win32_data.wparam) == WA_ACTIVE || LOWORD(win32_data.wparam) == WA_CLICKACTIVE) 
            {
                input->win32_window_active = true;
            } 
            else if (LOWORD(win32_data.wparam) == WA_INACTIVE) 
            {
                input->win32_window_active = false;
            }
        } break;

        case WM_MOUSEWHEEL: {
            input->mouse.cursor.z = (float)GET_WHEEL_DELTA_WPARAM(win32_data.wparam);
        } break;

        case WM_MOUSEMOVE: {
            f32 mx = (f32)GET_X_LPARAM(win32_data.lparam);
            f32 my = (f32)GET_Y_LPARAM(win32_data.lparam);
            input->mouse.cursor_delta.x = mx - input->mouse.cursor.x;
            input->mouse.cursor_delta.y = my - input->mouse.cursor.y;
            input->mouse.cursor.x = (f32)GET_X_LPARAM(win32_data.lparam);
            input->mouse.cursor.y = (f32)GET_Y_LPARAM(win32_data.lparam);
            if (input_down(input, Btn_LeftMouse)) 
            {
                input->mouse.cursor_drag_delta = input->mouse.cursor_delta;
            }
        } break;

        case WM_LBUTTONDOWN: {
            input->mouse.left.state |= (BUTTON_DOWN | BUTTON_HELD);
        } break;

        case WM_LBUTTONUP: {
            input->mouse.left.state &= ~(BUTTON_DOWN | BUTTON_HELD);
            input->mouse.cursor_delta = glm::vec3{0, 0, 0};
        } break;

        case WM_RBUTTONDOWN: {
            input->mouse.right.state |= (BUTTON_DOWN | BUTTON_HELD);
        } break;

        case WM_RBUTTONUP: {
            input->mouse.right.state &= ~(BUTTON_DOWN | BUTTON_HELD);
        } break;

        case WM_MBUTTONDOWN: {
            input->mouse.middle.state |= (BUTTON_DOWN | BUTTON_HELD);
        } break;

        case WM_MBUTTONUP: {
            input->mouse.middle.state &= ~(BUTTON_DOWN | BUTTON_HELD);
        } break;

        case WM_XBUTTONDOWN: {
            if (GET_XBUTTON_WPARAM(win32_data.wparam) == XBUTTON1) {
                input->mouse.x1.state |= (BUTTON_DOWN | BUTTON_HELD);
            }
            if (GET_XBUTTON_WPARAM(win32_data.wparam) == XBUTTON2) {
                input->mouse.x2.state |= (BUTTON_DOWN | BUTTON_HELD);
            }
        } break;

        case WM_XBUTTONUP: {

            if (GET_XBUTTON_WPARAM(win32_data.wparam) == XBUTTON1) {
                input->mouse.x1.state &= ~(BUTTON_DOWN | BUTTON_HELD);
            }
            if (GET_XBUTTON_WPARAM(win32_data.wparam) == XBUTTON2) {
                input->mouse.x2.state &= ~(BUTTON_DOWN | BUTTON_HELD);
            }
        } break;

        case WM_KEYDOWN: {
            /* LPARAM
             * bits 0-15:   repeat count
             * 16-23:        scan code (depends on OEM)
             * 24:          indicates extended key (right ctrl/alt)
             * 25-28:        reserved
             * 29:           context code, always 0 for WM_KEYDOWN
             * 30:           previous key state. 1 if down before msg was sent, 0 if the
             * key is up 31:           always 0 for wm_keydown
             */

            /* WPARAM
             * the VK code of non-system key
             */
            switch (win32_data.wparam) {
                case VK_SHIFT: {
                    input->keyboard.anyshift.state |= (BUTTON_DOWN | BUTTON_HELD);
                } break;

                case VK_BACK: {
                    input->keyboard.backspace.state |= (BUTTON_DOWN | BUTTON_HELD);
                } break;

                case VK_TAB: {
                    input->keyboard.tab_down.state |= (BUTTON_DOWN | BUTTON_HELD);
                } break;

                case VK_RETURN: {
                    input->keyboard.enter.state |= ( BUTTON_DOWN | BUTTON_HELD );

                } break;

                case VK_CONTROL: {
                    input->keyboard.anyctrl.state |= ( BUTTON_DOWN | BUTTON_HELD );
                } break;

                case VK_CAPITAL: {
                    input->keyboard.capslock.state |= ( BUTTON_DOWN | BUTTON_HELD );
                } break;

                default: {
                    u32 ccode = MapVirtualKey(win32_data.wparam, MAPVK_VK_TO_CHAR); // translate the keycode to a char
                    u16 repeat_count = (unsigned short)(win32_data.lparam); // take lower 16 bits
                    // b8 is_extended_key = (lparam & (1 << 24));   // key is either right control or alt key
                    // input->keyboard.keys[ccode].held = lparam & (1 << 30);
                    // input->keyboard.keys[ccode].down = true;
                    // input->keyboard.keys[ccode].character_value = ccode;
                    input->keyboard.keys[win32_data.wparam].state |= BUTTON_DOWN;
                    if(win32_data.lparam & (1 << 30))  // held bit
                    {
                        input->keyboard.keys[win32_data.wparam].state |= BUTTON_HELD;
                    }
                } break;
            }
        } break;

        case WM_KEYUP: {
            switch (win32_data.wparam) {
                case VK_SHIFT: {
                    input->keyboard.anyshift.state = 0;
                } break;

                case VK_BACK: {
                    input->keyboard.backspace.state = 0;
                } break;

                case VK_TAB: {
                    input->keyboard.tab_down.state = 0;
                } break;

                case VK_RETURN: {
                    input->keyboard.enter.state = 0;
                } break;

                case VK_CONTROL: {
                    input->keyboard.anyctrl.state = 0;
                } break;

                case VK_CAPITAL: {
                    input->keyboard.capslock.state = 0;
                } break;

                default: {
                    input->keyboard.keys[win32_data.wparam].state = 0;
                } break;
            }
        } break;

    }
    return LRESULT(0);
}

void input_begin_frame(InputSystem *input)
{
    if(input->using_raw_input)
    {
        input_process_win32_pending_rawinput_stream(input);
    }
    if(input->lock_cursor)
    {
        SetCursorPos(input->remember_x, input->remember_y);
    }
}

void input_end_frame(InputSystem* input) 
{
    // if a button is down by frame-end, set held bit?
    // or, if a button is down at the end of this frame, remember it and record the time as `time_pressed`.
    // if a button that was down is now up, remove it from the `held` tracker and untoggle its held bit
    // if a button that was down is still down, and `time_now - time_pressed` > `held_threshold_time`, toggle the held bit

    input->mouse.raw_input_cursor_delta = glm::vec3{ 0.0, 0.0, 0.0 };
    input->mouse.cursor_delta = glm::vec3{ 0.0, 0.0, 0.0 };
    input->mouse.cursor.z = 0.0; // clear the scrollwheel 

    // clear button flags that should only persist for 1 frame
    input->mouse.left.state &= ~(BUTTON_DBLCLICK | BUTTON_DOWN);
    input->mouse.right.state &= ~(BUTTON_DBLCLICK | BUTTON_DOWN);
    input->mouse.middle.state &= ~(BUTTON_DBLCLICK | BUTTON_DOWN);
    input->mouse.x1.state &= ~(BUTTON_DBLCLICK | BUTTON_DOWN);
    input->mouse.x2.state &= ~(BUTTON_DBLCLICK | BUTTON_DOWN);

    for(i32 i = 0; i < ARRAYSIZE(input->keyboard.keys); ++i)
    {
        input->keyboard.keys[i].state &= ~(BUTTON_DBLCLICK);
    }
}

bool is_msg_mouse(u32 msg) 
{
    switch (msg) 
    {
        case WM_MOUSEWHEEL:
        case WM_MOUSEMOVE:
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONDOWN:
        case WM_RBUTTONUP:
        case WM_MBUTTONDOWN:
        case WM_MBUTTONUP:
        case WM_XBUTTONDOWN:
        case WM_XBUTTONUP:
        case WM_RBUTTONDBLCLK:
        case WM_LBUTTONDBLCLK:
        case WM_MBUTTONDBLCLK:
        case WM_XBUTTONDBLCLK:
            return true;

        default:
            return false;
    }
}

void input_debug_ui(InputSystem* input) 
{
    glm::vec3 pos = input_cursorpos(input);
    glm::vec3 delta_pos = input_cursor_delta(input);
    glm::vec3 raw = input_rawinput_reading(input);

    ImGui::Text("cursor_pos:    %f  %f  %f", VECTOR3_PARTS(pos));
    ImGui::Text("cursor_delta:  %f  %f  %f", VECTOR3_PARTS(delta_pos));
    ImGui::Text("raw_input:     %f  %f  %f", VECTOR3_PARTS(raw));
    ImGui::Text("left         down: %d  held: %d  2xclick: %d",   BUTTON_PARTS(input->mouse.left));                                  
    ImGui::Text("right        down: %d  held: %d  2xclick: %d",   BUTTON_PARTS(input->mouse.right));                                 
    ImGui::Text("middle       down: %d  held: %d  2xclick: %d",   BUTTON_PARTS(input->mouse.middle));                                
    ImGui::Text("x1           down: %d  held: %d  2xclick: %d",   BUTTON_PARTS(input->mouse.x1));                                    
    ImGui::Text("x2           down: %d  held: %d  2xclick: %d",   BUTTON_PARTS(input->mouse.x2));

    ImGui::Separator();
    ImGui::Text("Keyboard state");

    for(int i = 0; i < ARRAYSIZE(input->keyboard.keys); ++i)
    {
            ImGui::Text("key %d (%c): down: %d  held: %d  2xclick: %d",
                        i,
                        (char)i,
                        BUTTON_PARTS(input->keyboard.keys[i]));
    }
}

b8 input_down(InputSystem *input, u32 button) 
{
    if(button > USER_BUTTON_DEF)
    {
        if(button == Btn_LeftMouse)
        {
            return button_down(input->mouse.left);
        }
        else if(button == Btn_RightMouse)
        {
            return button_down(input->mouse.right);
        }
        else if(button == Btn_MiddleMouse)
        {
            return button_down(input->mouse.middle);
        }
        else if(button == Btn_MouseSidebutton1)
        {
            return button_down(input->mouse.x1);
        }
        else if(button == Btn_MouseSidebutton2)
        {
            return button_down(input->mouse.x2);
        }
        else if(button == Btn_CapsLock)
        {
            return button_down(input->keyboard.capslock);
        }
        else if(button == Btn_Shift)
        {
            return button_down(input->keyboard.anyshift);
        }
        else if(button == Btn_Control)
        {
            return button_down(input->keyboard.anyctrl);
        }
        else if(button == Btn_Enter)
        {
            return button_down(input->keyboard.enter);
        }
        else 
        {
            return 0;
        }
    }
    else if(button >= 0 && button < USER_BUTTON_DEF)
    {
        return button_down(input->keyboard.keys[button]);
    }
    else
    {
        LOG_WARNING("checking state of undefined button... {}", button);
        return false;
    }
}

b8 input_held(InputSystem *input, u32 button)
{
    if(button > USER_BUTTON_DEF)
    {
        if(button == Btn_LeftMouse)
        {
            return button_held(input->mouse.left);
        }
        else if(button == Btn_RightMouse)
        {
            return button_held(input->mouse.right);
        }
        else if(button == Btn_MiddleMouse)
        {
            return button_held(input->mouse.middle);
        }
        else if(button == Btn_MouseSidebutton1)
        {
            return button_held(input->mouse.x1);
        }
        else if(button == Btn_MouseSidebutton2)
        {
            return button_held(input->mouse.x2);
        }
        else if(button == Btn_CapsLock)
        {
            return button_held(input->keyboard.capslock);
        }
        else if(button == Btn_Shift)
        {
            return button_held(input->keyboard.anyshift);
        }
        else if(button == Btn_Control)
        {
            return button_held(input->keyboard.anyctrl);
        }
        else if(button == Btn_Enter)
        {
            return button_held(input->keyboard.enter);
        }
        else
        {
            return 0;
        }
    }
    else if(button >= 0 && button < USER_BUTTON_DEF)
    {
        return button_held(input->keyboard.keys[button]);
    }
    else
    {
        LOG_WARNING("checking state of undefined button... {}", button);
        return false;
    }

}

void input_hide_cursor(InputSystem* input)
{
    if(input->showing_cursor == 0)
    {
        while(ShowCursor(0) >= -1);
    }
    input->showing_cursor = false;
}

void input_show_cursor(InputSystem* input)
{
    if(input->showing_cursor == 1)
    {
        while(ShowCursor(1) < 0);
    }
    input->showing_cursor = true;
}

void input_lock_cursor(InputSystem* input)
{
    POINT old_mouse;

    if(!input->lock_cursor)
    { 
        ::GetCursorPos(&old_mouse); // need global mouse pos
        input->remember_x = f32(old_mouse.x);
        input->remember_y = f32(old_mouse.y);
        input->lock_cursor = true;
    }
}

void input_unlock_cursor(InputSystem* input)
{
    if(input->lock_cursor)
        SetCursorPos(input->remember_x, input->remember_y);
    input->lock_cursor = false;
}


glm::vec3 input_cursorpos(InputSystem *input) { return input->mouse.cursor; }
glm::vec3 input_cursor_delta(InputSystem *input) { return input->mouse.cursor_delta; }
glm::vec3 input_cursor_drag_distance(InputSystem *input) { return input->mouse.cursor_drag_delta; }
glm::vec3 input_rawinput_reading(InputSystem *input) { return input->mouse.raw_input_cursor_delta; }

f32 input_mouse_wheel(InputSystem *input) { return input->mouse.cursor.z; }
