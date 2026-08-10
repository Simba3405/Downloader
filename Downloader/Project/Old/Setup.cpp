#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>

using namespace std;

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "ole32.lib") 
#pragma comment(lib, "uuid.lib")

// ========== 配置常量 ==========
const string DOCKER_INSTALLER = "DockerDesktopInstaller.exe";  // 你的安装包文件名
const string SEARXNG_IMAGE = "searxng/searxng";
const string SEARXNG_CONTAINER = "searxng";
const int SEARXNG_PORT = 8080;

// ========== 工具函数 ==========

// 设置控制台颜色
void setColor(int color) {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, color);
}

void resetColor() { setColor(7); }
void printOK(const string& msg) { setColor(10); cout << "[OK] " << msg << endl; resetColor(); }
void printInfo(const string& msg) { setColor(11); cout << "[*] " << msg << endl; resetColor(); }
void printError(const string& msg) { setColor(12); cout << "[X] " << msg << endl; resetColor(); }
void printWarn(const string& msg) { setColor(14); cout << "[!] " << msg << endl; resetColor(); }

// 执行命令并返回输出
string execCommand(const string& cmd, int& exitCode) {
    SECURITY_ATTRIBUTES sa = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };
    HANDLE hRead, hWrite;
    CreatePipe(&hRead, &hWrite, &sa, 0);
    
    STARTUPINFO si = { sizeof(STARTUPINFO) };
    si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
    si.hStdOutput = hWrite;
    si.hStdError = hWrite;
    si.wShowWindow = SW_HIDE;
    
    PROCESS_INFORMATION pi = { 0 };
    
    string fullCmd = "cmd.exe /c " + cmd;
    BOOL success = CreateProcess(NULL, (LPSTR)fullCmd.c_str(), NULL, NULL, TRUE,
                                  CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
    
    string output;
    if (success) {
        CloseHandle(hWrite);
        char buf[4096];
        DWORD read;
        while (ReadFile(hRead, buf, sizeof(buf)-1, &read, NULL) && read > 0) {
            buf[read] = '\0';
            output += buf;
        }
        WaitForSingleObject(pi.hProcess, INFINITE);
        DWORD ec;
        GetExitCodeProcess(pi.hProcess, &ec);
        exitCode = (int)ec;
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        exitCode = -1;
    }
    CloseHandle(hRead);
    return output;
}

// 检查 Docker 是否已安装
bool checkDockerInstalled() {
    int exitCode;
    string result = execCommand("docker --version", exitCode);
    return (exitCode == 0 && result.find("Docker") != string::npos);
}

// 获取程序所在目录
string getProgramDir() {
    char path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    string strPath(path);
    size_t pos = strPath.find_last_of("\\");
    return strPath.substr(0, pos);
}

// 获取桌面路径
string getDesktopPath() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_DESKTOP, NULL, 0, path))) {
        return string(path);
    }
    return "";
}

// 获取用户目录
string getUserProfile() {
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, path))) {
        return string(path);
    }
    return "";
}

// 创建快捷方式
bool createShortcut(const string& targetPath, const string& shortcutPath, 
                    const string& description, const string& iconPath = "") {
    CoInitialize(NULL);
    
    IShellLink* pShellLink = NULL;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                                    IID_IShellLink, (LPVOID*)&pShellLink);
    
    if (SUCCEEDED(hres)) {
        pShellLink->SetPath(targetPath.c_str());
        pShellLink->SetDescription(description.c_str());
        pShellLink->SetWorkingDirectory(getProgramDir().c_str());
        
        if (!iconPath.empty()) {
            pShellLink->SetIconLocation(iconPath.c_str(), 0);
        }
        
        IPersistFile* pPersistFile = NULL;
        hres = pShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&pPersistFile);
        
        if (SUCCEEDED(hres)) {
            wstring wShortcutPath(shortcutPath.begin(), shortcutPath.end());
            hres = pPersistFile->Save(wShortcutPath.c_str(), TRUE);
            pPersistFile->Release();
        }
        pShellLink->Release();
    }
    
    CoUninitialize();
    return SUCCEEDED(hres);
}

// 等待文件出现（用于检测安装进度）
bool waitForFile(const string& filePath, int timeoutSeconds) {
    for (int i = 0; i < timeoutSeconds; i++) {
        if (GetFileAttributes(filePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
            return true;
        }
        Sleep(1000);
    }
    return false;
}

// 检查端口是否被占用
bool checkPortAvailable(int port) {
    int exitCode;
    string result = execCommand("netstat -ano | findstr :" + to_string(port), exitCode);
    return result.empty();  // 空表示端口未被占用
}

// ========== 主功能函数 ==========

// 1. 安装 Docker
bool installDocker() {
    string installerPath = getProgramDir() + "\\" + DOCKER_INSTALLER;
    
    if (GetFileAttributes(installerPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        printError("Docker installer not found: " + installerPath);
        printInfo("Please place " + DOCKER_INSTALLER + " in the same folder as this program.");
        return false;
    }
    
    printInfo("Starting Docker Desktop installation...");
    printInfo("Installer: " + installerPath);
    printWarn("This may take several minutes. Please wait...");
    
    // 静默安装 Docker Desktop
    string installCmd = "\"" + installerPath + "\" install --quiet";
    int exitCode;
    string result = execCommand(installCmd, exitCode);
    
    if (exitCode != 0) {
        printError("Docker installation failed with code: " + to_string(exitCode));
        return false;
    }
    
    printOK("Docker Desktop installed successfully!");
    printInfo("Waiting for Docker service to start...");
    
    // 等待 Docker 就绪
    int attempts = 0;
    while (attempts < 60) {  // 最多等 60 秒
        if (checkDockerInstalled()) {
            printOK("Docker is ready!");
            return true;
        }
        Sleep(1000);
        attempts++;
        cout << ".";
    }
    
    printError("Docker failed to start within 60 seconds.");
    printInfo("Please restart computer and run this program again.");
    return false;
}

// 2. 配置 SearXNG
bool setupSearXNG() {
    printInfo("Setting up SearXNG...");
    
    // 检查端口
    if (!checkPortAvailable(SEARXNG_PORT)) {
        printWarn("Port " + to_string(SEARXNG_PORT) + " is in use. Stopping existing container...");
        int ignore;
        execCommand("docker rm -f " + SEARXNG_CONTAINER, ignore);
    }
    
    // 创建配置目录
    string configDir = getUserProfile() + "\\searxng-data";
    CreateDirectory(configDir.c_str(), NULL);
    
    // 写 settings.yml
    string settingsPath = configDir + "\\settings.yml";
    ofstream settingsFile(settingsPath);
    if (!settingsFile.is_open()) {
        printError("Cannot create settings file: " + settingsPath);
        return false;
    }
    
    settingsFile << R"(general:
  debug: false
  instance_name: "Local SearXNG"

search:
  safe_search: 0
  autocomplete: ""
  default_lang: "zh"
  formats:
    - html
    - json

server:
  secret_key: "auto_config_123456789"
  bind_address: "0.0.0.0"
  port: 8080
  limiter: false
  public_instance: false

ui:
  static_use_hash: true

outgoing:
  request_timeout: 15.0
  max_request_timeout: 20.0
  pool_connections: 100
  pool_maxsize: 20
  enable_http2: true

engines:
  - name: baidu
    engine: baidu
    shortcut: bd
    disabled: false
    timeout: 10.0

  - name: bing
    engine: bing
    shortcut: bi
    disabled: false
    timeout: 10.0

  - name: google
    engine: google
    shortcut: go
    disabled: true

  - name: duckduckgo
    engine: duckduckgo
    shortcut: ddg
    disabled: true

  - name: wikipedia
    engine: wikipedia
    shortcut: wp
    disabled: true

redis:
  url: false
)";
    settingsFile.close();
    printOK("SearXNG configuration written to: " + settingsPath);
    
    // 拉取镜像并启动容器
    printInfo("Pulling SearXNG image (this may take a while)...");
    int exitCode;
    string pullResult = execCommand("docker pull " + SEARXNG_IMAGE, exitCode);
    
    if (exitCode != 0) {
        printError("Failed to pull SearXNG image.");
        printInfo("Please check your internet connection.");
        return false;
    }
    
    printOK("Image pulled successfully.");
    
    // 启动容器
    string runCmd = "docker run -d --name " + SEARXNG_CONTAINER +
                    " -p " + to_string(SEARXNG_PORT) + ":" + to_string(SEARXNG_PORT) +
                    " -v \"" + configDir + ":/etc/searxng\"" +
                    " -e \"BASE_URL=http://localhost:" + to_string(SEARXNG_PORT) + "/\"" +
                    " " + SEARXNG_IMAGE;
    
    string runResult = execCommand(runCmd, exitCode);
    
    if (exitCode != 0) {
        printError("Failed to start SearXNG container.");
        return false;
    }
    
    printOK("SearXNG container started!");
    printInfo("Container ID: " + runResult.substr(0, 12));
    
    // 等待服务就绪
    printInfo("Waiting for SearXNG to be ready...");
    Sleep(3000);
    
    // 测试访问
    string testResult = execCommand(
        "curl -s -o nul -w \"%{http_code}\" http://localhost:" + to_string(SEARXNG_PORT), 
        exitCode
    );
    
    if (testResult == "200") {
        printOK("SearXNG is running at http://localhost:" + to_string(SEARXNG_PORT));
        return true;
    } else {
        printWarn("SearXNG may need more time to start.");
        printInfo("Please wait 10 seconds and test manually.");
        return true;  // 容器的启动可能慢一些
    }
}

// 3. 创建快捷方式
bool createDesktopShortcut() {
    printInfo("Creating desktop shortcut...");
    
    string targetPath = getProgramDir() + "\\Downloader.exe";  // 你的主程序名
    string desktopPath = getDesktopPath();
    string shortcutPath = desktopPath + "\\Downloader.lnk";
    
    if (GetFileAttributes(targetPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
        printWarn("Main program not found: " + targetPath);
        printInfo("Please ensure Downloader.exe is in the same folder.");
        // 仍然创建快捷方式，指向当前目录
        targetPath = getProgramDir() + "\\main file.exe";  // 备用名称
    }
    
    if (createShortcut(targetPath, shortcutPath, "AI Download Assistant")) {
        printOK("Shortcut created: " + shortcutPath);
        return true;
    } else {
        printError("Failed to create shortcut.");
        return false;
    }
}

// 4. 保存配置到主程序
bool saveConfigToMainProgram() {
    printInfo("Saving SearXNG configuration to main program...");
    
    // 创建配置文件，主程序启动时读取
    string configPath = getProgramDir() + "\\searxng.config";
    ofstream configFile(configPath);
    if (!configFile.is_open()) {
        printWarn("Cannot write config file.");
        return false;
    }
    
    configFile << "http://localhost:" << SEARXNG_PORT << endl;
    configFile.close();
    
    printOK("Configuration saved: " + configPath);
    return true;
}

// ========== 主函数 ==========

int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    system("cls");
    setColor(11);
    cout << R"(
    ___         __        __              __ 
   /   | __  __/ /_____  / /_____  ____  / /_
  / /| |/ / / / __/ __ \/ __/ __ \/ __ \/ __/
 / ___ / /_/ / /_/ /_/ / /_/ /_/ / /_/ / /_  
/_/  |_\__,_/\__/\____/\__/\____/ .___/\__/  
                               /_/            
)" << endl;
    resetColor();
    
    cout << "========================================" << endl;
    cout << "  AI Downloader - Auto Configuration" << endl;
    cout << "========================================" << endl << endl;
    
    // 步骤 1: 检查/安装 Docker
    printInfo("Step 1: Checking Docker installation...");
    
    if (checkDockerInstalled()) {
        printOK("Docker is already installed.");
    } else {
        printWarn("Docker not found. Starting installation...");
        if (!installDocker()) {
            printError("Docker installation failed. Cannot continue.");
            system("pause");
            return 1;
        }
    }
    
    // 步骤 2: 配置 SearXNG
    printInfo("\nStep 2: Configuring SearXNG search engine...");
    
    if (!setupSearXNG()) {
        printError("SearXNG setup failed.");
        system("pause");
        return 1;
    }
    
    // 步骤 3: 保存配置
    printInfo("\nStep 3: Saving configuration...");
    saveConfigToMainProgram();
    
    // 步骤 4: 创建快捷方式
    printInfo("\nStep 4: Creating desktop shortcut...");
    createDesktopShortcut();
    
    // 完成
    cout << "\n========================================" << endl;
    printOK("Auto configuration completed successfully!");
    cout << "========================================" << endl;
    printInfo("SearXNG is running at: http://localhost:8080");
    printInfo("You can now start the Downloader program.");
    printInfo("");
    printWarn("Note: If SearXNG shows 'Internal Server Error',");
    printWarn("      wait 10 seconds and restart the container:");
    printWarn("      docker restart searxng");
    
    system("pause");
    return 0;
}
