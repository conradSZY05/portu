#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <Windows.h>
#include <winreg.h>
#include <windef.h>
#include <limits>
#include <conio.h>
#include <map>

std::map<int, std::string> keyNames = {
    {72,  "UP"},
    {80,  "DOWN"},
    {75,  "LEFT"},
    {77,  "RIGHT"},
    {71,  "HOME"},
    {79,  "END"},
    {73,  "PAGE_UP"},
    {81,  "PAGE_DOWN"},
    {82,  "INSERT"},
    {83,  "DELETE"},
    {59,  "F1"},
    {60,  "F2"},
    {61,  "F3"},
    {62,  "F4"},
    {63,  "F5"},
    {64,  "F6"},
    {65,  "F7"},
    {66,  "F8"},
    {67,  "F9"},
    {68,  "F10"}
};

void printHeader() 
{
    std::cout << 
    "########################################" << std::endl <<
    "#                                      #" << std::endl <<
    "#     ____   ___  ____ _____ _   _     #" << std::endl <<
    "#    |  _ \\ / _ \\|  _ \\_   _| | | |    #" << std::endl <<
    "#    | |_) | | | | |_) || | | | | |    #" << std::endl <<
    "#    |  __/| |_| |  _ < | | | |_| |    #" << std::endl <<
    "#    |_|    \\___/|_| \\_\\|_|  \\___/     #" << std::endl <<
    "#                                      #" << std::endl <<
    "########################################" << std::endl << std::endl;
}

std::vector<std::string> getPorts() 
{
    std::vector<std::string> ports;
    HKEY hKey;
    LONG lResult;

    // open key
    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("HARDWARE\\DEVICEMAP\\SERIALCOMM"), 0, KEY_READ, &hKey);
    if(lResult != ERROR_SUCCESS) {
        std::cout << "error opening key." << std::endl;
        return ports;
    }

    
    DWORD dwIndex = 0;
    char keyName[256];
    byte dataValue[256];
    DWORD keyNameLength = sizeof(keyName);
    DWORD dataValueLength = sizeof(dataValue);
    int portCount = 1;

    std::cout << std::endl << "\x1b[1m\x1b[4m" << "available ports: " << "\x1b[22m\x1b[24m" << std::endl;

    // enumerate and read subkeys (first result 0 index then incremenet)
    lResult = RegEnumValueA(hKey, dwIndex, keyName, &keyNameLength, NULL, NULL, dataValue, &dataValueLength);
    while (lResult == ERROR_SUCCESS) {
        keyNameLength = sizeof(keyName);
        dataValueLength = sizeof(dataValue);
        
        std::cout << portCount++ << ". " << (char*)dataValue << std::endl;
        ports.push_back((char*)dataValue);
        lResult = RegEnumValueA(hKey, ++dwIndex, keyName, &keyNameLength, NULL, NULL, dataValue, &dataValueLength);
    }

    /*if (lResult == ERROR_NO_MORE_ITEMS) {
        std::cout << std::endl << "subkeys all enumerated successfully." << std::endl;
    }*/

    RegCloseKey(hKey);

    return ports;
}

void serialize(std::string port, int baud, std::map<int, std::string> keyMap) 
{

}

void getConfig(std::vector<std::string> ports)
{
    size_t portSize = ports.size();

    // get port
    int sel;
    std::cout << "select port (1-" << portSize << "): ";
    
    while(true) {
        std::cin >> sel;
        if(std::cin.fail() || sel < 1 || sel > portSize) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "invalid try again (1-" << portSize << "): ";
            continue;
        }
        break;
    }
    std::string port = ports[sel - 1];

    // get baud rate
    int baud;
    std::cout << std::endl << "enter baud rate (9600, 115200 etc): ";

    while(true) {
        std::cin >> baud;
        if(std::cin.fail()) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "invalid try again: ";
            continue;
        }
        break;
    }

    // get key mapping
    std::map<int, std::string> keyMap;
    int inputCode;
    std::string sendStr;

    while(true) {
        std::cout << "enter key to configure (ESC to finish, no capital letters): ";
        inputCode = _getch(); 
        if(inputCode == 224 || inputCode == 0) { // first keycode in _getch is 224 for arrow keys etc and 0 for F1, F2 ...
            inputCode = _getch();
        }
        if(inputCode == 27) break;

        std::cout << (keyNames.count(inputCode) ? keyNames[inputCode] : std::string(1, (char)inputCode)) << std::endl;
        if(!keyNames.count(inputCode)) {
            std::cout << "send string: ";
            std::cin >> sendStr;
        }

        if(keyMap.count(inputCode)) {
            std::cout << "already used " << (keyNames.count(inputCode) ? keyNames[inputCode] : std::string(1, (char)inputCode)) << " try a different key. " << std::endl;
            continue;
        }
        
        keyMap.insert({inputCode, (keyNames.count(inputCode) ? keyNames[inputCode] : sendStr)});
    }

    // serialize to config
    serialize(port, baud, keyMap);
}


int main ()
{
    std::cout << "\033[2J\033[H";
    printHeader();

    std::vector<std::string> portsAvailable = getPorts();
    getConfig(portsAvailable);


    return 0;
}