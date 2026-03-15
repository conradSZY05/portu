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
#include <fstream>
#include <iostream>
#include <sys/stat.h>
#include <fileapi.h>

std::map<int, char> keyNames = {
    {72,  'U'},
    {80,  'D'},
    {75,  'L'},
    {77,  'R'},
    {71,  'H'},
    {79,  'E'},
    {73,  'P'},
    {81,  'N'},
    {82,  'I'},
    {83,  'D'},
    {59,  '1'},
    {60,  '2'},
    {61,  '3'},
    {62,  '4'},
    {63,  '5'},
    {64,  '6'},
    {65,  '7'},
    {66,  '8'},
    {67,  '9'},
    {68,  '0'}
};

void printHeader() 
{
    std::cout << "\033[2J\033[H";
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

void printKeyMap(std::map<int, char> keyMap)
{
    std::cout << "key pressed = message sent" << std::endl;
    for(auto it = keyMap.begin(); it != keyMap.end(); it++) {
        std::cout << (keyNames.count(it->first) ? keyNames[it->first] : (char)it->first) << " = " << it->second << std::endl;
    }
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

void serialize(std::string port, int baud, std::map<int, char> keyMap) 
{
    std::ofstream file;
    file.open("config.txt");
    file << port << std::endl << baud;
    for(auto it = keyMap.begin(); it != keyMap.end(); it++) {
        file << std::endl << (keyNames.count(it->first) ? keyNames[it->first] : (char)it->first) << "=" << it->second;
    }

    file.close();
}

bool deserialize(std::string& port, int& baud, std::map<int, char>& keyMap)
{
    //check config.txt exists
    std::string filename = "config.txt";
    struct stat buf;
    if(stat(filename.c_str(), &buf) == -1)
        return false;
 
    std::ifstream file;
    file.open(filename);
    std::string line;
    int count = 0;
    while(getline(file, line)) {
        //port is first then baud then keymap
        if(count == 0) {
            port = line;
            count++;
        } else if(count == 1) {
            baud = std::stoi(line, nullptr, 10);
            count++;
        } else {
            bool special = false;
            int splitIndex = line.find("=");
            char keyStr = line.substr(0, splitIndex)[0];
            int key = 0;
            for(auto it = keyNames.begin(); it != keyNames.end(); it++) {
                if(keyStr == it->second) {
                    key = it->first;
                    special = true;
                    break;
                }
            }
            char value = line.substr(splitIndex+1)[0];
            keyMap.insert({special ? key : (int)keyStr, value});
        }
    }

    file.close();
    return true;
}

void getConfig(std::vector<std::string> ports, std::string& port, int& baud, std::map<int, char>& keyMap)
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
    port = ports[sel - 1];

    // get baud rate
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
    int inputCode;
    char sendCh;

    while(true) {
        std::cout << "enter key to configure (ESC to finish, no capital letters): ";
        inputCode = _getch(); 
        if(inputCode == 224 || inputCode == 0) { // first keycode in _getch is 224 for arrow keys etc and 0 for F1, F2 ...
            inputCode = _getch();
        }
        if(inputCode == 27) break;

        std::cout << (keyNames.count(inputCode) ? keyNames[inputCode] : (char)inputCode) << std::endl;
        if(!keyNames.count(inputCode)) {
            std::string inpStr;
            std::cout << "enter char: ";
            while(true) {
                std::cin >> inpStr;
                if(inpStr.length() > 1) {
                    std::cout << "try again, enter char: ";
                    continue;
                }
                break;
            }
            sendCh = inpStr[0];
        }

        if(keyMap.count(inputCode)) {
            std::cout << "already used " << (keyNames.count(inputCode) ? keyNames[inputCode] : (char)inputCode) << " try a different key. " << std::endl;
            continue;
        }
        
        keyMap.insert({inputCode, (keyNames.count(inputCode) ? keyNames[inputCode] : sendCh)});
    }

    // serialize to config
    serialize(port, baud, keyMap);
}

bool openSerial(std::string port, int baud, HANDLE &hCommPort) 
{
    std::string portPath = "\\\\.\\" + port;
    hCommPort = CreateFileA(portPath.c_str(), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if(hCommPort == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB dcbCommPort;
    SecureZeroMemory(&dcbCommPort, sizeof(DCB));
    dcbCommPort.DCBlength = sizeof(DCB);
    // get comm port settings
    GetCommState(hCommPort, &dcbCommPort);
    // congifure port (baud, data length, parity, stop bits)
    dcbCommPort.BaudRate = baud == 75 ? BAUD_075 :
                           110 ? BAUD_110 :
                           115200 ? BAUD_115200 :
                           1200 ? BAUD_1200 :
                           128000 ? BAUD_128K :
                           14400 ? BAUD_14400 :
                           150 ? BAUD_150 :
                           1800 ? BAUD_1800 :
                           19200 ? BAUD_19200 :
                           2400 ? BAUD_2400 :
                           300 ? BAUD_300 :
                           38400 ? BAUD_38400 :
                           4800 ? BAUD_4800 :
                           56000 ? BAUD_56K : 
                           57600 ? BAUD_57600 :
                           600 ? BAUD_600 :
                           7200 ? BAUD_7200 :
                           9600 ? BAUD_9600 : BAUD_134_5;
    dcbCommPort.ByteSize = 8;
    dcbCommPort.Parity = NOPARITY;
    dcbCommPort.StopBits = ONESTOPBIT;
    //update the port
    if(!SetCommState(hCommPort, &dcbCommPort)) {
        return false;
    }

    // set timeout values for serial writing
    COMMTIMEOUTS commPortTimeouts;
    commPortTimeouts.ReadIntervalTimeout = 1;
    commPortTimeouts.ReadTotalTimeoutMultiplier = 1;
    commPortTimeouts.ReadTotalTimeoutMultiplier = 1;
    commPortTimeouts.WriteTotalTimeoutConstant = 1;
    commPortTimeouts.WriteTotalTimeoutMultiplier = 1;
    // update timeouts
    if(!SetCommTimeouts(hCommPort, &commPortTimeouts)) {
        return false;
    }

    return true;
}

int main ()
{
    printHeader();

    std::vector<std::string> portsAvailable = getPorts();

    // check if want to use last config
    char configSelection;
    std::cout << std::endl << "use last config? (y/n): ";
    while(true) {
        std::cin >> configSelection;
        if(configSelection != 'y' && configSelection != 'n') {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << std::endl << "invalid try again (y/n): ";
            continue;
        }
        break;
    }

    std::string port;
    int baud;
    std::map<int, char> keyMap;

    if(configSelection == 'n') {
        getConfig(portsAvailable, port, baud, keyMap);
    } else {
        if(!deserialize(port, baud, keyMap)) {
            std::cout << "unable to get last config, try again" << std::endl;
            getConfig(portsAvailable, port, baud, keyMap);
        }
    }

    //open serial connection
    HANDLE hCommPort;
    if(!openSerial(port, baud, hCommPort)) {
        std::cout << "unable to open serial connection at port " << port << ". exiting. ";
        return 0;
    } 

    // enter main loop, print keymap, wait for keys pressed and send via open port
    printHeader(); 
    printKeyMap(keyMap);
    std::cout << std::endl << std::endl << "send data (ESC to close): ";
    while(true) {
        DWORD numBytesWritten;
        int keyToSend;
        keyToSend = _getch();
        if(keyToSend == 224 || keyToSend == 0) { //special keys listed in keynames
            keyToSend = _getch();
        }
        if(keyToSend == 27) { //esc
            break;
        }
        if(keyMap.count(keyToSend) != 0) { //only write if actually configured
            char valueToSend = keyMap[keyToSend];
            std::cout << valueToSend << ", ";
            WriteFile(hCommPort, &valueToSend, 1, &numBytesWritten, NULL);
        }
    }
    CloseHandle(hCommPort);

    return 0;
}