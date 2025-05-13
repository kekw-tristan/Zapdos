#include "Windows.h"

class cInput
{
    public:

        static void OnKeyDown(WPARAM key) { m_keys[key] = true; }
        static void OnKeyUp(WPARAM key) { m_keys[key] = false; }

        static bool IsKeyDown(WPARAM key) { return m_keys[key]; }

        static void OnMouseMove(int x, int y) { m_mouseX = x; m_mouseY = y; }
        static void OnMouseButtonDown(WPARAM button) { m_mouseButtons[button] = true; }
        static void OnMouseButtonUp(WPARAM button) { m_mouseButtons[button] = false; }

        static int GetMouseX() { return m_mouseX; }
        static int GetMouseY() { return m_mouseY; }
        static bool IsMouseButtonDown(WPARAM button) { return m_mouseButtons[button]; }

    private:

        static inline bool m_keys[256] = { false };
        static inline bool m_mouseButtons[5] = { false };
        static inline int m_mouseX = 0, m_mouseY = 0;
};