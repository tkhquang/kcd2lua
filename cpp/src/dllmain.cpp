#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include "MinHook.h"
#include <psapi.h>
#include <fstream>
#include <functional>
#include <string>
#include <sstream>
#include <vector>
#include <cstdint>
#include <iostream>
#include <fcntl.h>
#include <io.h>
#include <deque>
#include <mutex>
#include <filesystem>

extern "C" {
#include "lua.h"
#include "lauxlib.h"
}

constexpr uint16_t TCP_PORT = 28771;
const char* PCALL_SIG = "? ? ? ? ? 57 48 83 EC 40 33 C0 41 8B F8 44 8B D2 48 8B D9 45 85 C9 74 0C 41 8B D1";

struct Pattern {
    std::vector<uint8_t> bytes;
    std::vector<bool> mask;
};

Pattern CreatePattern(const char* signature) {
    Pattern pattern;
    std::string sig(signature);

    for (size_t i = 0; i < sig.length(); i += 2) {
        if (sig[i] == ' ') { i--; continue; }
        if (sig[i] == '?') {
            pattern.bytes.push_back(0);
            pattern.mask.push_back(false);
            if (sig[i + 1] == '?') i++;
        }
        else {
            char byte[3] = { sig[i], sig[i + 1], 0 };
            pattern.bytes.push_back(static_cast<uint8_t>(strtol(byte, nullptr, 16)));
            pattern.mask.push_back(true);
        }
    }
    return pattern;
}

uintptr_t FindPattern(const uint8_t* data, const size_t dataSize, const Pattern& pattern) {
    for (size_t i = 0; i < dataSize - pattern.bytes.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.bytes.size(); j++) {
            if (pattern.mask[j] && data[i + j] != pattern.bytes[j]) {
                found = false;
                break;
            }
        }
        if (found) return i;
    }
    return 0;
}

HANDLE g_ConsoleHandle = NULL;
bool g_ConsoleEnabled = false;

bool CheckForConsoleArg() {
    LPWSTR cmdLine = GetCommandLineW();
    int argc;
    LPWSTR* argv = CommandLineToArgvW(cmdLine, &argc);

    if (argv != NULL) {
        for (int i = 1; i < argc; i++) {
            if (_wcsicmp(argv[i], L"-console") == 0) {
                LocalFree(argv);
                return true;
            }
        }
        LocalFree(argv);
    }
    return false;
}

void InitConsole() {
    if (!CheckForConsoleArg()) {
        g_ConsoleEnabled = false;
        return;
    }

    g_ConsoleEnabled = true;
    AllocConsole();
    g_ConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTitleA("Mod Debug Console");

    COORD bufferSize = { 120, 9000 };
    SetConsoleScreenBufferSize(g_ConsoleHandle, bufferSize);

    SMALL_RECT windowSize = { 0, 0, 119, 30 };
    SetConsoleWindowInfo(g_ConsoleHandle, TRUE, &windowSize);
}

template<typename T>
std::string formatHex(T value) {
    std::stringstream ss;
    ss << "0x" << std::hex << std::uppercase << value;
    return ss.str();
}

template<typename T>
std::string formatPtr(T* ptr) {
    return formatHex(reinterpret_cast<uintptr_t>(ptr));
}

void Log(const std::string& message)
{
    // Write to console if enabled
    if (g_ConsoleEnabled && g_ConsoleHandle != NULL) {
        std::string consoleMsg = message + "\n";
        DWORD written;
        WriteConsoleA(g_ConsoleHandle, consoleMsg.c_str(), consoleMsg.length(), &written, NULL);
    }

    // Always write to log file
    std::ofstream logFile("./kcd2lua.log", std::ios_base::app);
    if (logFile.is_open())
    {
        logFile << message << std::endl;
        logFile.close();
    }
    else if (g_ConsoleEnabled && g_ConsoleHandle != NULL)
    {
        const char* error = "[ERROR] Could not open log file.\n";
        DWORD written;
        WriteConsoleA(g_ConsoleHandle, error, strlen(error), &written, NULL);
    }
}

template<typename... Args>
void LogFormat(const std::string& format, Args... args) {
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), format.c_str(), args...);
    Log(buffer);
}

struct ExecutionResult {
    bool success;
    std::string message;
    bool ready;
};

std::deque<std::string> scriptQueue;
std::mutex queueMutex;
bool isExecutingCustomScripts = false;

ExecutionResult currentResult = { true, "", false };
std::condition_variable resultCondition;
std::mutex resultMutex;

SOCKET listenSocket = INVALID_SOCKET;

void pushFileToQueue(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(queueMutex);
    scriptQueue.push_back(filepath);
    LogFormat("[INFO] Pushed file to queue: %s", filepath.c_str());
}

DWORD WINAPI TCPServerThread(LPVOID)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        Log("[ERROR] Failed to initialize WinSock.");
        return 1;
    }

    listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        Log("[ERROR] Failed to create socket.");
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(TCP_PORT);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
    {
        Log("[ERROR] Failed to bind socket.");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
    {
        Log("[ERROR] Failed to listen on socket.");
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    LogFormat("[INFO] TCP Server listening on 127.0.0.1:%d", TCP_PORT);

    while (true)
    {
        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET)
        {
            Log("[ERROR] Failed to accept connection.");
            continue;
        }

        Log("[INFO] Client connected!");

        while (true)
        {
            Log("[INFO] Waiting for message...");

            // Read message length (4 bytes)
            uint32_t messageLength = 0;
            int bytesReceived = 0;

            try {
                bytesReceived = recv(clientSocket, (char*)&messageLength, sizeof(messageLength), 0);
            }
            catch (const std::exception& e) {
                LogFormat("[ERROR] Exception during recv (message length): %s", e.what());
                break;
            }
            catch (...) {
                Log("[ERROR] Unknown exception during recv (message length)");
                break;
            }

            if (bytesReceived <= SOCKET_ERROR) {
                int error = WSAGetLastError();
                switch (error) {
                    case WSAECONNRESET:
                        LogFormat("[WARN] Connection reset by peer (error: %d)", error);
                        break;
                    case WSAETIMEDOUT:
                        LogFormat("[WARN] Connection timed out (error: %d)", error);
                        break;
                    default:
                        LogFormat("[ERROR] recv failed with error: %d", error);
                        break;
                }
                break;  // Exit the inner loop
            }

            if (bytesReceived == 0) {
                break;  // Connection closed gracefully
            }

            if (bytesReceived != sizeof(messageLength)) {
                Log("[ERROR] Failed to read message length");
                break;
            }

            LogFormat("[INFO] Expecting message of length %d", messageLength);

            // Read the file paths
            std::vector<char> buffer(messageLength);
            size_t totalReceived = 0;

            while (totalReceived < messageLength) {
                bytesReceived = recv(clientSocket, buffer.data() + totalReceived,
                                messageLength - totalReceived, 0);

                if (bytesReceived <= 0) {
                    Log("[ERROR] Connection error while reading message");
                    break;
                }

                totalReceived += bytesReceived;
            }

            if (totalReceived == messageLength) {
                std::string fileList(buffer.data());
                std::stringstream ss(fileList);
                std::string filePath;
                
                Log("[INFO] Processing file list...");

                // Reset execution result
                {
                    std::lock_guard<std::mutex> lock(resultMutex);
                    currentResult = { true, "", false };
                }

                while (std::getline(ss, filePath, ',')) {
                    if (filePath.empty()) continue;
                    pushFileToQueue(filePath);
                }
                
                std::string response = "Files queued for execution\n";
                send(clientSocket, response.c_str(), response.length(), 0);

                // Wait for the signal that execution completed
                {
                    std::unique_lock<std::mutex> lock(resultMutex);
                    resultCondition.wait(lock, [] { return currentResult.ready; });
                    
                    // Send the result back to vscode
                    std::string response = currentResult.message;
                    send(clientSocket, response.c_str(), response.length(), 0);
                }
            }

            Log("[INFO] Files executed");
        }

        closesocket(clientSocket);
        Log("[INFO] Client disconnected");
    }

    return 0;
}

namespace hooks
{
    typedef int32_t(__cdecl* lua_pcall_t)(lua_State* L, int32_t nargs, int32_t nresults, int32_t errfunc);
    lua_pcall_t plua_pcall = nullptr;

    int32_t lua_pcall_hook(lua_State* L_, int32_t nargs, int32_t nresults, int32_t errfunc)
    {
        if (isExecutingCustomScripts) {
            return plua_pcall(L_, nargs, nresults, errfunc);
        }

        isExecutingCustomScripts = true;
        bool hadErrors = false;
        std::string errorMessages;

        while (!scriptQueue.empty()) {
            std::string currentFile;
            
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                currentFile = scriptQueue.front();
                scriptQueue.pop_front();
            }
            
            LogFormat("[INFO] Loading file: %s", currentFile.c_str());

            if (luaL_dofile(L_, currentFile.c_str()) != 0) {
                const char* errorMsg = lua_tostring(L_, -1);
                std::string error = errorMsg ? errorMsg : "Unknown Lua error";

                size_t lastSlash = currentFile.find_last_of("/\\");
                std::string filename = (lastSlash == std::string::npos) ? 
                currentFile : currentFile.substr(lastSlash + 1);

                LogFormat("[ERROR] Failed to execute file %s: %s", filename.c_str(), error.c_str());
                errorMessages += "Error in " + filename + ": " + error + "\n";
                hadErrors = true;
                lua_pop(L_, 1);
            }
            else {
                LogFormat("[INFO] Successfully executed file: %s", currentFile.c_str());
            }
        }

        isExecutingCustomScripts = false;

        // Execute the original pcall
        int32_t result = plua_pcall(L_, nargs, nresults, errfunc);

        // Update execution result
        {
            std::lock_guard<std::mutex> lock(resultMutex);
            currentResult.success = !hadErrors;
            currentResult.message = hadErrors ? errorMessages : "All scripts executed successfully";
            currentResult.ready = true;
            resultCondition.notify_one();
        }

        return result;
    }
}

bool VerifyAddress(void* addr, const char* name) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0) {
        LogFormat("[ERROR] Failed to query memory for %s", name);
        return false;
    }

    if (mbi.State != MEM_COMMIT) {
        LogFormat("[ERROR] %s address is not committed memory", name);
        return false;
    }

    if (!(mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY))) {
        LogFormat("[ERROR] %s address is not executable", name);
        return false;
    }

    return true;
}

const Pattern PCALL_PATTERN = CreatePattern(PCALL_SIG);
void hook()
{
    HMODULE hModule = GetModuleHandleW(L"WHGame.dll");
    if (!hModule) {
        Log("[ERROR] Failed to get WHGame.dll module handle");
        return;
    }

    MODULEINFO modInfo;
    if (!GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        Log("[ERROR] Failed to get module information");
        return;
    }

    const uint8_t* moduleBase = static_cast<const uint8_t*>(modInfo.lpBaseOfDll);
    const size_t moduleSize = modInfo.SizeOfImage;

    LogFormat("[INFO] WHGame.dll base address: %s", formatPtr(moduleBase).c_str());

    uintptr_t pcall_offset = FindPattern(moduleBase, moduleSize, PCALL_PATTERN);

    if (!pcall_offset) {
        Log("[ERROR] Could not find lua_pcall pattern");
        return;
    }

    LogFormat("[INFO] Found offsets - lua_pcall: %s",
        formatHex(pcall_offset).c_str());

    void* pcall_addr = (void*)(reinterpret_cast<uintptr_t>(moduleBase) + pcall_offset);

    LogFormat("[INFO] Found lua_pcall at: %s", formatPtr(pcall_addr).c_str());

    MH_STATUS status = MH_Initialize();
    if (status != MH_OK) {
        Log("[ERROR] Failed to initialize MinHook");
        return;
    }

    if (MH_CreateHook(pcall_addr,
                        reinterpret_cast<LPVOID>(&hooks::lua_pcall_hook),
                        reinterpret_cast<LPVOID*>(&hooks::plua_pcall)) != MH_OK) {
        Log("[ERROR] Failed to create lua_pcall hook");
        return;
    }

    if (MH_EnableHook(MH_ALL_HOOKS) != MH_OK) {
        Log("[ERROR] Failed to enable hooks");
        return;
    }

    Log("[INFO] Hooks setup successfully");
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, [](LPVOID param) -> DWORD {
            Log("\n\n[INFO] Mod loaded");
            InitConsole();
            if (g_ConsoleEnabled) {
                Log("[INFO] Mod console initialized");
            }
            hook();
            CreateThread(nullptr, 0, TCPServerThread, nullptr, 0, nullptr);
            return 0;
        }, nullptr, 0, nullptr);
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        if (listenSocket != INVALID_SOCKET)
        {
            closesocket(listenSocket);
            WSACleanup();
        }
        if (g_ConsoleEnabled && g_ConsoleHandle != NULL) {
            FreeConsole();
            g_ConsoleHandle = NULL;
        }
    }
    return TRUE;
}
