#pragma once

#include "runtime/core/math/moyu_math2.h"
#include <vector>

namespace MoYu
{
    enum class GameCommand : unsigned int
    {
        forward  = 1 << 0,                 // W
        backward = 1 << 1,                 // S
        left     = 1 << 2,                 // A
        right    = 1 << 3,                 // D
        jump     = 1 << 4,                 // not implemented yet
        squat    = 1 << 5,                 // not implemented yet
        sprint   = 1 << 6,                 // LEFT SHIFT
        fire     = 1 << 7,                 // not implemented yet
        invalid  = (unsigned int)(1 << 31) // lost focus
    };

    // Keyboard buttons enumeration
    enum class KeyBoardButton
    {
        // Alphabetic keys
        A = 65,
        B = 66,
        C = 67,
        D = 68,
        E = 69,
        F = 70,
        G = 71,
        H = 72,
        I = 73,
        J = 74,
        K = 75,
        L = 76,
        M = 77,
        N = 78,
        O = 79,
        P = 80,
        Q = 81,
        R = 82,
        S = 83,
        T = 84,
        U = 85,
        V = 86,
        W = 87,
        X = 88,
        Y = 89,
        Z = 90,

        // Numeric keys
        KEY_0 = 48,
        KEY_1 = 49,
        KEY_2 = 50,
        KEY_3 = 51,
        KEY_4 = 52,
        KEY_5 = 53,
        KEY_6 = 54,
        KEY_7 = 55,
        KEY_8 = 56,
        KEY_9 = 57,

        // Special keys
        SPACE = 32,
        ESCAPE = 256,
        ENTER = 257,
        TAB = 258,
        BACKSPACE = 259,
        INSERT = 260,
        //DELETE = 261,
        RIGHT = 262,
        LEFT = 263,
        DOWN = 264,
        UP = 265,
        PAGE_UP = 266,
        PAGE_DOWN = 267,
        HOME = 268,
        END = 269,
        CAPS_LOCK = 280,
        SCROLL_LOCK = 281,
        NUM_LOCK = 282,
        PRINT_SCREEN = 283,
        PAUSE = 284,

        // Function keys
        F1 = 290,
        F2 = 291,
        F3 = 292,
        F4 = 293,
        F5 = 294,
        F6 = 295,
        F7 = 296,
        F8 = 297,
        F9 = 298,
        F10 = 299,
        F11 = 300,
        F12 = 301,

        // Modifier keys
        LEFT_SHIFT = 340,
        LEFT_CONTROL = 341,
        LEFT_ALT = 342,
        LEFT_SUPER = 343,
        RIGHT_SHIFT = 344,
        RIGHT_CONTROL = 345,
        RIGHT_ALT = 346,
        RIGHT_SUPER = 347,

        // Other keys
        MENU = 348
    };

    extern unsigned int k_complement_control_command;

    class InputSystem 
    {

    public:
        void onKey(int key, int scancode, int action, int mods);
        void onCursorPos(double current_cursor_x, double current_cursor_y);

        void initialize();
        void tick();
        void clear();

        // New methods for checking key states
        bool isKeyPressing(KeyBoardButton key) const;
        bool isKeyReleased(KeyBoardButton key) const;

        int m_cursor_delta_x {0};
        int m_cursor_delta_y {0};

        float m_cursor_delta_yaw {0};
        float m_cursor_delta_pitch {0};
        float m_cursor_delta_scroll {0}; // Add scroll delta

        void resetGameCommand() { m_game_command = 0; }
        unsigned int getGameCommand() const { return m_game_command; }

    private:
        void onKeyInGameMode(int key, int scancode, int action, int mods);
        void onMouseButton(int button, int action, int mods);
        void onScroll(double xoffset, double yoffset);

        void calculateCursorDeltaAngles();

        unsigned int m_game_command {0};

        // Track key states (implementation in cpp file)

        int m_last_cursor_x {0};
        int m_last_cursor_y {0};
    };
} // namespace MoYu