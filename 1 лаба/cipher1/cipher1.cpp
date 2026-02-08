#include <iostream>
#include <string>
#include <windows.h>
#include <fstream>
#include <ctime>
#include <locale>
#include <algorithm>
#include <codecvt>
#include <vector>

using namespace std;

HANDLE hSerial;
ofstream logFile;
ofstream binFile;

const int CHUNK_SIZE = 50;

/**
 * Преобразует строку из UTF-8 в кодировку CP866
 * @param utf8_str Строка в кодировке UTF-8
 * @return Строка в кодировке CP866
 */
string utf8_to_cp866(const string& utf8_str) {
    if (utf8_str.empty()) return "";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), NULL, 0);
    wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), (int)utf8_str.size(), &wstr[0], size_needed);

    size_needed = WideCharToMultiByte(866, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    string result(size_needed, 0);
    WideCharToMultiByte(866, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, NULL, NULL);

    return result;
}

/**
 * Преобразует строку из CP866 в кодировку UTF-8
 * @param cp866_str Строка в кодировке CP866
 * @return Строка в кодировке UTF-8
 */
string cp866_to_utf8(const string& cp866_str) {
    if (cp866_str.empty()) return "";

    int size_needed = MultiByteToWideChar(866, 0, cp866_str.c_str(), (int)cp866_str.size(), NULL, 0);
    wstring wstr(size_needed, 0);
    MultiByteToWideChar(866, 0, cp866_str.c_str(), (int)cp866_str.size(), &wstr[0], size_needed);

    size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), NULL, 0, NULL, NULL);
    string result(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), (int)wstr.size(), &result[0], size_needed, NULL, NULL);

    return result;
}

/**
 * Улучшенный фильтр для исправления ошибок дешифрования
 * Исправляет заглавные "Н" и другие артефакты дешифрования
 * @param text Текст для обработки в кодировке CP866
 * @return Исправленный текст
 */
string fix_decryption_errors(const string& text) {
    string result = text;

    for (size_t i = 0; i < result.length(); i++) {
        
        if (result[i] == '<') {
            result[i] = '\xEF'; 
        }
        if (result[i] == '>') {
            result[i] = '\xF0';
        }

        if (result[i] == 'Н') {
            bool is_punctuation = false;

            if (i > 0 && result[i - 1] >= 'а' && result[i - 1] <= 'я') {
                if (i + 1 < result.length()) {
                    if (result[i + 1] == ' ' || result[i + 1] == '\n' || result[i + 1] == '\r') {
                        result[i] = ',';
                        is_punctuation = true;
                    }
                    else if (result[i + 1] >= 'а' && result[i + 1] <= 'я') {
                        result.replace(i, 1, ", ");
                        i++;
                        is_punctuation = true;
                    }
                }
                else {
                    result[i] = ',';
                    is_punctuation = true;
                }
            }

            if (!is_punctuation && result[i] == 'Н') {
                result[i] = ' ';
            }
        }

        if (result[i] == '\x95' || result[i] == '\xFA') {
            bool before_lowercase = (i > 0 && result[i - 1] >= 'а' && result[i - 1] <= 'я');
            bool after_space = (i + 1 < result.length() && result[i + 1] == ' ');

            if (before_lowercase && after_space) {
                result[i] = '\x97'; 
            }
            else if (before_lowercase) {
                result[i] = ',';
            }
            else {
                result[i] = ' ';
            }
        }
    }

    for (size_t i = 0; i < result.length(); i++) {
        if (i > 0) {
            if (result[i - 1] == '?' && result[i] >= 'А' && result[i] <= 'Я') {
                result.insert(i, " ");
                i++;
            }
            else if (result[i - 1] == ',' && result[i] >= 'а' && result[i] <= 'я') {
                result.insert(i, " ");
                i++;
            }
        }
    }

    size_t pos = 0;
    while ((pos = result.find("  ", pos)) != string::npos) {
        result.erase(pos, 1);
    }

    return result;
}

/**
 * Отправляет команду в ESP и получает ответ
 * @param komanda Команда для отправки
 * @param timeout_ms Таймаут ожидания ответа в миллисекундах
 * @return Ответ от ESP
 */
string otpravit_komandu_v_esp(const string& komanda, int timeout_ms = 5000) {
    DWORD skolko_zapisano;
    string dannye_s_perenosom = komanda + "\n";

    PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);

    WriteFile(hSerial, dannye_s_perenosom.c_str(),
        dannye_s_perenosom.length(), &skolko_zapisano, NULL);

    Sleep(500);

    string vse_dannye;
    char bufer[1024];
    DWORD skolko_prochitano;
    DWORD start = GetTickCount();

    while (GetTickCount() - start < timeout_ms) {
        if (ReadFile(hSerial, bufer, sizeof(bufer) - 1, &skolko_prochitano, NULL)) {
            if (skolko_prochitano > 0) {
                bufer[skolko_prochitano] = '\0';
                vse_dannye += bufer;

                if (vse_dannye.find("REZULTAT|") != string::npos ||
                    vse_dannye.find("OSHIBKA|") != string::npos) {
                    break;
                }
            }
        }
        Sleep(50);
    }

    if (vse_dannye.find("REZULTAT|") != string::npos) {
        size_t gde = vse_dannye.find("REZULTAT|");
        string otvet = vse_dannye.substr(gde + 9);
        size_t gde_perenos = otvet.find('\n');
        if (gde_perenos != string::npos) otvet = otvet.substr(0, gde_perenos);

        otvet.erase(remove(otvet.begin(), otvet.end(), '\r'), otvet.end());
        otvet.erase(remove(otvet.begin(), otvet.end(), '\n'), otvet.end());

        return otvet;
    }

    return "OSHIBKA";
}

/**
 * Разбивает текст на части и шифрует их по отдельности
 * @param text Текст для шифрования
 * @param shift Сдвиг для алгоритма
 * @return Зашифрованный текст
 */
string shifrovat_po_chastam(string text, int shift) {
    string result = "";
    int chankov = (text.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int obrabotano = 0;

    for (size_t i = 0; i < text.length(); i += CHUNK_SIZE) {
        string chunk = text.substr(i, CHUNK_SIZE);
        string komanda = "SHIFR|" + to_string(shift) + "|" + chunk;

        if (logFile.is_open()) {
            logFile << "[" << time(nullptr) << "] SHIFR chunk " << i / CHUNK_SIZE + 1 << "/" << chankov << endl;
        }

        string otvet = otpravit_komandu_v_esp(komanda, 10000);

        if (otvet == "OSHIBKA") {
            return "Oshibka: ESP ne otvechaet pri shifrovanii chunka " + to_string(i / CHUNK_SIZE + 1);
        }

        result += otvet + " ";
        obrabotano++;

        if (chankov > 5 && obrabotano % 5 == 0) {
            cout << "Obrabotano " << obrabotano << " iz " << chankov << " chankov..." << endl;
        }

        Sleep(300);
    }

    return result;
}

/**
 * Разбивает текст на части и дешифрует их по отдельности
 * @param text Текст для дешифрования
 * @param shift Сдвиг для алгоритма
 * @return Дешифрованный текст
 */
string deshifrovat_po_chastam(string text, int shift) {
    string result = "";
    int chankov = (text.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
    int obrabotano = 0;

    for (size_t i = 0; i < text.length(); i += CHUNK_SIZE) {
        string chunk = text.substr(i, CHUNK_SIZE);
        string komanda = "RASHIFR|" + to_string(shift) + "|" + chunk;

        if (logFile.is_open()) {
            logFile << "[" << time(nullptr) << "] RASHIFR chunk " << i / CHUNK_SIZE + 1 << "/" << chankov << endl;
        }

        string otvet = otpravit_komandu_v_esp(komanda, 10000);

        if (otvet == "OSHIBKA") {
            return "Oshibka: ESP ne otvechaet pri deshifrovanii chunka " + to_string(i / CHUNK_SIZE + 1);
        }

        result += otvet;
        obrabotano++;

        if (chankov > 5 && obrabotano % 5 == 0) {
            cout << "Obrabotano " << obrabotano << " iz " << chankov << " chankov..." << endl;
        }

        Sleep(300);
    }

    return result;
}

/**
 * Функция шифрования текста с использованием внешнего устройства ESP
 * @param text Текст для шифрования в кодировке CP866
 * @param shift Сдвиг для алгоритма шифрования
 * @return Зашифрованный текст
 */
string shifrovat(string text, int shift) {
    if (hSerial == INVALID_HANDLE_VALUE) return "Oshibka: port ne otkryt";
    if (text.length() > CHUNK_SIZE) {
        cout << "Obrabotka po chastam (" << (text.length() + CHUNK_SIZE - 1) / CHUNK_SIZE << " chankov)..." << endl;
        return shifrovat_po_chastam(text, shift);
    }

    string komanda = "SHIFR|" + to_string(shift) + "|" + text;

    if (logFile.is_open()) {
        logFile << "[" << time(nullptr) << "] SHIFR|" << text.substr(0, min(50, (int)text.length())) << "|" << shift << endl;
    }

    string otvet = otpravit_komandu_v_esp(komanda, 10000);

    if (otvet == "OSHIBKA") {
        return "Oshibka: ESP ne otvechaet";
    }

    return otvet;
}

/**
 * Функция дешифрования текста с использованием внешнего устройства ESP
 * @param text Текст для дешифрования в кодировке CP866
 * @param shift Сдвиг для алгоритма дешифрования
 * @return Расшифрованный текст с применённым фильтром исправления
 */
string deshifrovat(string text, int shift) {
    if (hSerial == INVALID_HANDLE_VALUE) return "Oshibka: port ne otkryt";

    if (text.length() > CHUNK_SIZE) {
        cout << "Obrabotka po chastam (" << (text.length() + CHUNK_SIZE - 1) / CHUNK_SIZE << " chankov)..." << endl;
        string result = deshifrovat_po_chastam(text, shift);

        if (result.find("Oshibka") == string::npos) {
            result = fix_decryption_errors(result);
        }

        return result;
    }

    string komanda = "RASHIFR|" + to_string(shift) + "|" + text;

    if (logFile.is_open()) {
        logFile << "[" << time(nullptr) << "] RASHIFR|" << text.substr(0, min(50, (int)text.length())) << "|" << shift << endl;
    }

    string otvet = otpravit_komandu_v_esp(komanda, 10000);

    if (otvet == "OSHIBKA") {
        return "Oshibka: ESP ne otvechaet";
    }

    otvet = fix_decryption_errors(otvet);

    return otvet;
}

/**
 * Читает содержимое файла и преобразует его в кодировку CP866
 * @param filename Имя файла для чтения
 * @return Содержимое файла в кодировке CP866
 */
string prochitat_iz_faila(string filename) {
    ifstream file(filename, ios::binary);
    if (!file.is_open()) {
        return "";
    }

    file.seekg(0, ios::end);
    size_t size = file.tellg();
    file.seekg(0, ios::beg);

    string result(size, '\0');
    file.read(&result[0], size);
    file.close();

    string result_cp866 = utf8_to_cp866(result);

    return result_cp866;
}

/**
 * Записывает текст в файл с преобразованием в UTF-8
 * @param filename Имя файла для записи
 * @param text Текст для записи в кодировке CP866
 * @return true если запись успешна, false в противном случае
 */
bool zapisat_v_fail(string filename, string text) {
    string text_utf8 = cp866_to_utf8(text);

    ofstream file(filename, ios::binary);
    if (!file.is_open()) {
        return false;
    }

    file.write(text_utf8.c_str(), text_utf8.length());
    file.close();
    return true;
}

/**
 * Открывает последовательный порт для связи с ESP
 * @param port_name Имя порта (например, "COM8")
 * @return true если порт успешно открыт, false в противном случае
 */
bool otkrit_port(string port_name) {
    string polnoe_imya = "\\\\.\\" + port_name;
    hSerial = CreateFileA(polnoe_imya.c_str(),
        GENERIC_READ | GENERIC_WRITE,
        0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    DCB param = { 0 };
    param.DCBlength = sizeof(param);

    if (!GetCommState(hSerial, &param)) {
        CloseHandle(hSerial);
        return false;
    }

    param.BaudRate = 115200;
    param.ByteSize = 8;
    param.StopBits = ONESTOPBIT;
    param.Parity = NOPARITY;

    if (!SetCommState(hSerial, &param)) {
        CloseHandle(hSerial);
        return false;
    }

    COMMTIMEOUTS time = { 0 };
    time.ReadIntervalTimeout = 50;
    time.ReadTotalTimeoutConstant = 50;
    time.ReadTotalTimeoutMultiplier = 10;
    time.WriteTotalTimeoutConstant = 50;
    time.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &time)) {
        CloseHandle(hSerial);
        return false;
    }

    return true;
}

/**
 * Закрывает последовательный порт
 */
void zakrit_port() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
}

/**
 * Очищает буфер последовательного порта
 */
void ochistit_bufer() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        PurgeComm(hSerial, PURGE_RXCLEAR | PURGE_TXCLEAR);
    }
}

/**
 * Проверяет подключение к ESP через последовательный порт
 * @param port Имя порта для проверки
 * @return true если ESP отвечает, false в противном случае
 */
bool proverit_esp(string port) {
    if (!otkrit_port(port)) {
        cout << "Oshibka: ne poluchaetsya otkryt port " << port << endl;
        return false;
    }

    ochistit_bufer();
    Sleep(2000);

    string otvet;
    char bufer[256];
    DWORD skolko_prochitano;
    DWORD start = GetTickCount();
    bool poluchil_otvet = false;

    while (GetTickCount() - start < 10000) {
        if (ReadFile(hSerial, bufer, sizeof(bufer) - 1, &skolko_prochitano, NULL)) {
            if (skolko_prochitano > 0) {
                bufer[skolko_prochitano] = '\0';
                otvet += bufer;

                if (otvet.find("GOTOV") != string::npos) {
                    poluchil_otvet = true;
                    break;
                }
            }
        }
        Sleep(100);
    }

    if (!poluchil_otvet) {
        zakrit_port();
        cout << "Oshibka: ESP ne otvechaet" << endl;
        return false;
    }

    return true;
}

/**
 * Выводит справочную информацию об использовании программы
 */
void pokazat_pomosh() {
    cout << "Kak ispolzovat:" << endl;
    cout << "  --mode encrypt/decrypt   Chto delat (shifrovat ili deshifrovat)" << endl;
    cout << "  --offset N               Na skolko smeshat (chislo)" << endl;
    cout << "  --text \"text\"          Tekst dlya obrabotki" << endl;
    cout << "  --file filename.txt      Fail s tekstom dlya obrabotki" << endl;
    cout << "  --port COMx              Port ESP (naprimer, COM8)" << endl;
    cout << "  --help                   Pokazat etu pomosh" << endl;
}

/**
 * Обрабатывает аргументы командной строки
 * @param argc Количество аргументов
 * @param argv Массив аргументов
 */
void rezhim_iz_komandnoi_stroki(int argc, char* argv[]) {
    string mode, text, port, filename;
    int offset = 0;
    bool isFileInput = false;

    for (int i = 1; i < argc; i++) {
        string arg = argv[i];

        if (arg == "--help") {
            pokazat_pomosh();
            return;
        }
        else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        }
        else if (arg == "--offset" && i + 1 < argc) {
            offset = atoi(argv[++i]);
        }
        else if (arg == "--text" && i + 1 < argc) {
            text = argv[++i];
            text = utf8_to_cp866(text);
        }
        else if (arg == "--file" && i + 1 < argc) {
            filename = argv[++i];
            isFileInput = true;
        }
        else if (arg == "--port" && i + 1 < argc) {
            port = argv[++i];
        }
    }

    if (mode.empty() || offset == 0 || port.empty()) {
        cout << "Oshibka: ne vse ukazano" << endl;
        pokazat_pomosh();
        return;
    }

    if (isFileInput) {
        text = prochitat_iz_faila(filename);
        if (text.empty()) {
            cout << "Oshibka: ne udalos prochitat fail " << filename << endl;
            return;
        }
    }
    else if (text.empty()) {
        cout << "Oshibka: ne ukazan text ili fail" << endl;
        pokazat_pomosh();
        return;
    }

    if (!proverit_esp(port)) return;

    string result;
    if (mode == "encrypt") {
        result = shifrovat(text, offset);
    }
    else if (mode == "decrypt") {
        result = deshifrovat(text, offset);
    }
    else {
        cout << "Oshibka: plohoi rezhim" << endl;
        zakrit_port();
        return;
    }

    if (isFileInput) {
        string output_filename;
        if (mode == "encrypt") {
            output_filename = filename + "_encrypted.txt";
        }
        else {
            output_filename = filename + "_decrypted.txt";
        }

        if (zapisat_v_fail(output_filename, result)) {
            cout << "Rezultat sohranen v fail: " << output_filename << endl;
        }
        else {
            cout << "Oshibka sohraneniya rezultata v fail" << endl;
        }
    }

    if (result.length() < 100) {
        cout << result << endl;
    }
    else {
        cout << "Rezultat poluchen. Dlina: " << result.length() << " simvolov" << endl;
        cout << "Pervye 100 simvolov: " << result.substr(0, 100) << endl;
    }

    zakrit_port();
}

/**
 * Запускает интерактивный режим работы с программой
 */
void interaktivnyi_rezhim() {
    SetConsoleCP(866);
    SetConsoleOutputCP(866);

    while (true) {
        cout << endl;
        cout << "1. Podklyuchitsya k ESP" << endl;
        cout << "0. Vyihti" << endl;
        cout << "Viberi: ";

        string vibor;
        getline(cin, vibor);

        if (vibor == "0") break;
        else if (vibor != "1") {
            cout << "Plohoi vibor" << endl;
            continue;
        }

        string port;
        cout << "Vvedi COM port (0 dlya otmeny): ";
        getline(cin, port);

        if (port == "0") continue;

        if (!proverit_esp(port)) continue;

        cout << "ESP podklyuchena" << endl;

        while (true) {
            cout << endl;
            cout << "1. Shifrovat" << endl;
            cout << "2. Deshifrovat" << endl;
            cout << "3. Rabota s failom" << endl;
            cout << "0. Otklyuchitsya" << endl;
            cout << "Viberi: ";

            getline(cin, vibor);

            if (vibor == "0") {
                zakrit_port();
                break;
            }
            else if (vibor == "3") {
                string filename;
                cout << "Vvedi imya faila: ";
                getline(cin, filename);

                string text = prochitat_iz_faila(filename);
                if (text.empty()) {
                    cout << "Oshibka: ne udalos prochitat fail" << endl;
                    continue;
                }

                cout << "Prochitano iz faila: " << text.length() << " simvolov" << endl;

                int chankov = (text.length() + CHUNK_SIZE - 1) / CHUNK_SIZE;
                if (text.length() > CHUNK_SIZE) {
                    cout << "Text budet obrabotan po chastam (" << chankov << " chankov)" << endl;
                }

                cout << "1. Shifrovat soderzhimoe faila" << endl;
                cout << "2. Deshifrovat soderzhimoe faila" << endl;
                cout << "0. Nazad" << endl;
                cout << "Viberi: ";

                string file_vibor;
                getline(cin, file_vibor);

                if (file_vibor == "0") continue;

                int offset;
                cout << "Vvedi smeshenie: ";
                string offset_str;
                getline(cin, offset_str);

                try {
                    offset = stoi(offset_str);
                }
                catch (...) {
                    cout << "Oshibka: smeshenie dolzhno byt chislom" << endl;
                    continue;
                }

                string result;
                string output_filename;

                if (file_vibor == "1") {
                    cout << "Shifruem... (obrabotka " << chankov << " chankov)" << endl;
                    result = shifrovat(text, offset);
                    output_filename = filename + "_encrypted.txt";
                }
                else if (file_vibor == "2") {
                    cout << "Deshifruem... (obrabotka " << chankov << " chankov)" << endl;
                    result = deshifrovat(text, offset);
                    output_filename = filename + "_decrypted.txt";
                }
                else {
                    cout << "Plohoi vibor" << endl;
                    continue;
                }

                if (result.find("Oshibka") != string::npos) {
                    cout << "Oshibka: " << result << endl;
                    continue;
                }

                if (zapisat_v_fail(output_filename, result)) {
                    cout << "Rezultat sohranen v fail: " << output_filename << endl;
                }
                else {
                    cout << "Oshibka sohraneniya rezultata v fail" << endl;
                }

                if (result.length() < 100) {
                    cout << "Poluchilos: " << result << endl;
                }
                else {
                    cout << "Poluchilos: " << result.length() << " simvolov" << endl;
                    cout << "Pervye 100 simvolov: " << result.substr(0, 100) << endl;
                }
                continue;
            }
            else if (vibor != "1" && vibor != "2") {
                cout << "Plohoi vibor" << endl;
                continue;
            }

            string text;
            cout << "Vvedi text (0 dlya otmeny): ";
            getline(cin, text);

            if (text == "0") continue;

            int offset;
            cout << "Vvedi smeshenie (0 dlya otmeny): ";
            string offset_str;
            getline(cin, offset_str);

            if (offset_str == "0") continue;

            try {
                offset = stoi(offset_str);
            }
            catch (...) {
                cout << "Oshibka: smeshenie dolzhno byt chislom" << endl;
                continue;
            }

            string result;
            if (vibor == "1") {
                cout << "Shifruem..." << endl;
                result = shifrovat(text, offset);
            }
            else {
                cout << "Deshifruem..." << endl;
                result = deshifrovat(text, offset);
            }

            cout << "Poluchilos: " << result << endl;
        }
    }

    cout << "Vyihti iz programmi" << endl;
}

/**
 * Главная функция программы
 * @param argc Количество аргументов командной строки
 * @param argv Массив аргументов командной строки
 * @return Код завершения программы
 */
int main(int argc, char* argv[]) {
    logFile.open("log.txt", ios::app);
    binFile.open("result.bin", ios::binary | ios::out | ios::trunc);

    if (argc > 1) {
        rezhim_iz_komandnoi_stroki(argc, argv);
    }
    else {
        interaktivnyi_rezhim();
    }

    if (logFile.is_open()) {
        logFile.close();
    }

    if (binFile.is_open()) {
        binFile.close();
    }

    return 0;
}