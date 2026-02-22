#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <ctime>

using namespace std;

/**
 * Алфавит, используемый для шифрования/дешифрования.
 * Содержит русские буквы (заглавные и строчные, включая ё),
 * цифры и основные знаки пунктуации.
 */
const wstring ALPHABET = L"абвгдеёжзийклмнопрстуфхцчшщъыьэюя"
L"АБВГДЕЁЖЗИЙКЛМНОПРСТУФХЦЧШЩЪЫЬЭЮЯ"
L"0123456789 .,!?;:\"'-()";

const int N = ALPHABET.size();

/* Преобразование UTF-8 <-> wstring */
wstring utf8_to_wstring(const string& utf8)
{
    if (utf8.empty()) return wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), nullptr, 0);
    wstring result(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), static_cast<int>(utf8.size()), &result[0], size);
    return result;
}

string wstring_to_utf8(const wstring& wstr)
{
    if (wstr.empty()) return string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
    string result(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &result[0], size, nullptr, nullptr);
    return result;
}

/* Логирование с поддержкой UTF-8 BOM */
void Log(const wstring& message)
{
    ofstream log("log.txt", ios::binary | ios::app);
    if (!log.is_open()) return;

    log.seekp(0, ios::end);
    if (log.tellp() == 0)
    {
        const unsigned char bom[] = { 0xEF, 0xBB, 0xBF };
        log.write(reinterpret_cast<const char*>(bom), 3);
    }

    time_t t = time(nullptr);
    struct tm tm;
    localtime_s(&tm, &t);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &tm);

    string utf8Message = wstring_to_utf8(message);
    log << "[" << timeStr << "] " << utf8Message << endl;
    log.close();
}

/**
 * Возвращает индекс символа в алфавите.
 * @param c Символ для поиска.
 * @return Индекс символа или -1, если символ не найден.
 */
int findIndex(wchar_t c)
{
    for (int i = 0; i < N; ++i)
    {
        if (ALPHABET[i] == c)
            return i;
    }
    return -1;
}

/**
 * Шифрует текст сдвигом (X + K) mod N.
 * @param text Открытый текст.
 * @param shift Сдвиг K (целое число).
 * @return Зашифрованный текст.
 */
wstring Encrypt(const wstring& text, int shift)
{
    wstring result;
    for (wchar_t c : text)
    {
        int idx = findIndex(c);
        if (idx != -1)
        {
            int newIdx = ((idx + shift) % N + N) % N;
            result += ALPHABET[newIdx];
        }
        else
        {
            result += c;
        }
    }
    return result;
}

/**
 * Дешифрует текст сдвигом (X - K) mod N.
 * @param text Зашифрованный текст.
 * @param shift Сдвиг K (целое число).
 * @return Расшифрованный текст.
 */
wstring Decrypt(const wstring& text, int shift)
{
    wstring result;
    for (wchar_t c : text)
    {
        int idx = findIndex(c);
        if (idx != -1)
        {
            int newIdx = ((idx - shift) % N + N) % N;
            result += ALPHABET[newIdx];
        }
        else
        {
            result += c;
        }
    }
    return result;
}

/* Чтение/запись файлов */
wstring ReadFileToWstring(const wstring& filename, bool& success)
{
    success = false;
    ifstream file(filename, ios::binary);
    if (!file.is_open()) return wstring();

    file.seekg(0, ios::end);
    streamsize size = file.tellg();
    file.seekg(0, ios::beg);

    string buffer(static_cast<size_t>(size), '\0');
    file.read(&buffer[0], size);
    file.close();

    success = true;
    return utf8_to_wstring(buffer);
}

bool WriteWstringToFile(const wstring& filename, const wstring& text)
{
    string utf8 = wstring_to_utf8(text);
    ofstream file(filename, ios::binary);
    if (!file.is_open()) return false;
    file.write(utf8.c_str(), utf8.size());
    file.close();
    return true;
}

/**
 * Выводит справочную информацию о программе.
 */
void PrintHelp()
{
    wcout << L"Использование: cipher.exe [--mode MODE] [--offset N] [--text \"TEXT\"] [--file FILE] [--help]\n"
        << L"  --mode encrypt/decrypt   Режим работы: шифрование или дешифрование.\n"
        << L"  --offset N                Сдвиг (целое число).\n"
        << L"  --text \"TEXT\"            Текст для обработки.\n"
        << L"  --file FILE               Файл с текстом для обработки.\n"
        << L"  --help                     Показать эту справку.\n"
        << L"Пример: cipher.exe --mode encrypt --offset 5 --text \"Привет, мир!\"\n"
        << L"Пример: cipher.exe --mode decrypt --offset 5 --file encrypted.txt\n";
}

/**
 * Разбирает аргументы командной строки и запускает соответствующий режим.
 * @param argc Количество аргументов.
 * @param argv Массив аргументов (широкие символы).
 */
void ProcessCommandLine(int argc, wchar_t* argv[])
{
    wstring mode, text, filename;
    int offset = 0;
    bool helpRequested = false;
    bool useFile = false;

    for (int i = 1; i < argc; ++i)
    {
        wstring arg = argv[i];
        if (arg == L"--help")
        {
            helpRequested = true;
            break;
        }
        else if (arg == L"--mode" && i + 1 < argc)
        {
            mode = argv[++i];
        }
        else if (arg == L"--offset" && i + 1 < argc)
        {
            offset = static_cast<int>(wcstol(argv[++i], nullptr, 10));
        }
        else if (arg == L"--text" && i + 1 < argc)
        {
            text = argv[++i];
        }
        else if (arg == L"--file" && i + 1 < argc)
        {
            filename = argv[++i];
            useFile = true;
        }
    }

    if (helpRequested)
    {
        PrintHelp();
        Log(L"Показана справка");
        return;
    }

    if (mode.empty() || offset == 0)
    {
        wcout << L"Ошибка: не указаны обязательные параметры --mode и --offset.\n";
        Log(L"Ошибка: не указаны обязательные параметры");
        PrintHelp();
        return;
    }

    if (useFile)
    {
        if (filename.empty())
        {
            wcout << L"Ошибка: не указано имя файла.\n";
            Log(L"Ошибка: не указано имя файла");
            return;
        }
        bool success;
        text = ReadFileToWstring(filename, success);
        if (!success)
        {
            wcout << L"Ошибка: не удалось прочитать файл " << filename << endl;
            Log(L"Ошибка чтения файла: " + filename);
            return;
        }
        Log(L"Файл прочитан: " + filename);
    }
    else if (text.empty())
    {
        wcout << L"Ошибка: не указан текст (--text) или файл (--file).\n";
        Log(L"Ошибка: не указан текст или файл");
        PrintHelp();
        return;
    }

    wstring result;
    if (mode == L"encrypt")
    {
        result = Encrypt(text, offset);
        Log(L"Выполнено шифрование (сдвиг " + to_wstring(offset) + L")");
    }
    else if (mode == L"decrypt")
    {
        result = Decrypt(text, offset);
        Log(L"Выполнено дешифрование (сдвиг " + to_wstring(offset) + L")");
    }
    else
    {
        wcout << L"Ошибка: неизвестный режим. Используйте encrypt или decrypt.\n";
        Log(L"Ошибка: неизвестный режим " + mode);
        return;
    }

    if (useFile)
    {
        size_t dotPos = filename.find_last_of(L'.');
        wstring baseName = (dotPos == wstring::npos) ? filename : filename.substr(0, dotPos);
        wstring outFilename = baseName + (mode == L"encrypt" ? L"_шифранул.txt" : L"_дешифранул.txt");

        if (WriteWstringToFile(outFilename, result))
        {
            wcout << L"Результат сохранён в файл: " << outFilename << endl;
            Log(L"Результат сохранён в файл: " + outFilename);
        }
        else
        {
            wcout << L"Ошибка при сохранении результата.\n";
            Log(L"Ошибка сохранения результата в файл: " + outFilename);
        }
    }
    else
    {
        wcout << L"Результат: " << result << endl;
        Log(L"Результат выведен на экран");
    }
}

/**
 * Интерактивный режим работы.
 */
void InteractiveMode()
{
    wcout << L"Интерактивный режим. Для выхода введите пустую строку в главном меню.\n";
    Log(L"Запущен интерактивный режим");

    while (true)
    {
        wcout << L"\nВыберите действие:\n"
            << L"1. Шифровать текст с клавиатуры\n"
            << L"2. Дешифровать текст с клавиатуры\n"
            << L"3. Работа с файлом\n"
            << L"0. Выход\n"
            << L"Ваш выбор: ";

        wstring choice;
        getline(wcin, choice);

        if (choice == L"0")
        {
            Log(L"Выход из интерактивного режима");
            break;
        }

        if (choice == L"1" || choice == L"2")
        {
            wcout << L"Введите текст: ";
            wstring text;
            getline(wcin, text);
            if (text.empty()) continue;

            wcout << L"Введите сдвиг (целое число): ";
            wstring offsetStr;
            getline(wcin, offsetStr);
            if (offsetStr.empty()) continue;

            int offset;
            try
            {
                offset = stoi(offsetStr);
            }
            catch (...)
            {
                wcout << L"Ошибка: сдвиг должен быть целым числом.\n";
                continue;
            }

            wstring result;
            if (choice == L"1")
            {
                result = Encrypt(text, offset);
                Log(L"Интерактивно: шифрование (сдвиг " + to_wstring(offset) + L")");
            }
            else
            {
                result = Decrypt(text, offset);
                Log(L"Интерактивно: дешифрование (сдвиг " + to_wstring(offset) + L")");
            }

            wcout << L"Результат:\n" << result << endl;
        }
        else if (choice == L"3")
        {
            wcout << L"Введите имя входного файла: ";
            wstring filename;
            getline(wcin, filename);
            if (filename.empty()) continue;

            bool success;
            wstring text = ReadFileToWstring(filename, success);
            if (!success)
            {
                wcout << L"Ошибка: не удалось прочитать файл.\n";
                Log(L"Интерактивно: ошибка чтения файла " + filename);
                continue;
            }
            wcout << L"Файл прочитан, размер: " << text.size() << L" символов.\n";

            wcout << L"Выберите действие с файлом:\n"
                << L"1. Зашифровать\n"
                << L"2. Расшифровать\n"
                << L"Ваш выбор: ";
            wstring act;
            getline(wcin, act);
            if (act != L"1" && act != L"2") continue;

            wcout << L"Введите сдвиг (целое число): ";
            wstring offsetStr;
            getline(wcin, offsetStr);
            if (offsetStr.empty()) continue;

            int offset;
            try
            {
                offset = stoi(offsetStr);
            }
            catch (...)
            {
                wcout << L"Ошибка: сдвиг должен быть целым числом.\n";
                continue;
            }

            wstring result;
            if (act == L"1")
            {
                result = Encrypt(text, offset);
                Log(L"Интерактивно: шифрование файла (сдвиг " + to_wstring(offset) + L")");
            }
            else
            {
                result = Decrypt(text, offset);
                Log(L"Интерактивно: дешифрование файла (сдвиг " + to_wstring(offset) + L")");
            }

            size_t dotPos = filename.find_last_of(L'.');
            wstring baseName = (dotPos == wstring::npos) ? filename : filename.substr(0, dotPos);
            wstring outFilename = baseName + (act == L"1" ? L"_шифранул.txt" : L"_дешифранул.txt");

            if (WriteWstringToFile(outFilename, result))
            {
                wcout << L"Результат сохранён в файл: " << outFilename << endl;
                Log(L"Результат сохранён в файл: " + outFilename);
            }
            else
            {
                wcout << L"Ошибка при сохранении результата.\n";
                Log(L"Ошибка сохранения результата в файл: " + outFilename);
            }
        }
        else
        {
            wcout << L"Неверный выбор. Попробуйте снова.\n";
        }
    }
}

/**
 * Точка входа в программу.
 * @param argc Количество аргументов командной строки.
 * @param argv Массив аргументов (широкие символы).
 * @return Код завершения.
 */
int wmain(int argc, wchar_t* argv[])
{
    setlocale(LC_ALL, "ru_RU.UTF-8");
    _setmode(_fileno(stdout), _O_U16TEXT);
    _setmode(_fileno(stdin), _O_U16TEXT);

    Log(L"Программа запущена");

    if (argc > 1)
    {
        ProcessCommandLine(argc, argv);
    }
    else
    {
        InteractiveMode();
    }

    Log(L"Программа завершена");
    return 0;
}