#pragma once
#include <windows.h>

namespace lsproxy {

class AddonManager;

class GuiManager {
public:
    static void StartGuiThread(AddonManager* manager);

private:
    static DWORD WINAPI GuiThread(LPVOID lpParam);
};

} // namespace lsproxy
