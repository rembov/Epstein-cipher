#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#include <cerrno>
#include <algorithm>

using namespace std;
using namespace chrono;

/**
 * Разлагает число на простые множители методом перебора делителей (пробное деление).
 * Оптимизация: проверка 2 и 3, далее шаг 6k±1.
 * @param n Число для факторизации (должно быть >= 2).
 * @return Вектор простых множителей (с повторениями).
 */
vector<int64_t> TrialDivision(int64_t n)
{
    vector<int64_t> factors;

    while (n % 2 == 0)
    {
        factors.push_back(2);
        n /= 2;
    }

    while (n % 3 == 0)
    {
        factors.push_back(3);
        n /= 3;
    }

    int64_t d = 5;
    while (d * d <= n)
    {
        while (n % d == 0)
        {
            factors.push_back(d);
            n /= d;
        }
        d += 2;
        while (n % d == 0)
        {
            factors.push_back(d);
            n /= d;
        }
        d += 4;
    }

    if (n > 1)
        factors.push_back(n);

    return factors;
}

/**
 * Разлагает число на простые множители методом факторизации Ферма.
 * Работает эффективно, когда множители близки к квадратному корню.
 * Для чётных чисел сначала выделяет степень двойки.
 * @param n Число для факторизации (должно быть >= 2).
 * @return Вектор простых множителей (с повторениями).
 */
vector<int64_t> FermatsMethod(int64_t n)
{
    vector<int64_t> factors;

    while (n % 2 == 0)
    {
        factors.push_back(2);
        n /= 2;
    }

    if (n == 1) return factors;

    bool maybePrime = true;
    if (n % 3 == 0) { factors.push_back(3); n /= 3; maybePrime = false; }
    int64_t smallPrimes[3] = { 5, 7, 11 };
    for (int64_t p : smallPrimes)
    {
        if (n % p == 0)
        {
            factors.push_back(p);
            n /= p;
            maybePrime = false;
            break;
        }
    }

    if (n == 1) return factors;

    if (maybePrime && n < 1000000) 
    {
        int64_t limit = static_cast<int64_t>(sqrt(n));
        bool isPrime = true;
        for (int64_t d = 3; d <= limit; d += 2)
        {
            if (n % d == 0)
            {
                isPrime = false;
                break;
            }
        }
        if (isPrime)
        {
            factors.push_back(n);
            return factors;
        }
    }

    int64_t a = static_cast<int64_t>(ceil(sqrt(n)));
    int64_t b2 = a * a - n;
    int64_t maxIter = 10000000; 
    int64_t iter = 0;

    while (iter < maxIter)
    {
        int64_t b = static_cast<int64_t>(sqrt(b2));
        if (b * b == b2)
        {
            int64_t factor1 = a - b;
            int64_t factor2 = a + b;

            if (factor1 == 1)
            {
                factors.push_back(n);
                return factors;
            }

            vector<int64_t> f1 = FermatsMethod(factor1);
            vector<int64_t> f2 = FermatsMethod(factor2);
            factors.insert(factors.end(), f1.begin(), f1.end());
            factors.insert(factors.end(), f2.begin(), f2.end());
            return factors;
        }
        a++;
        b2 = a * a - n;
        iter++;
    }

    vector<int64_t> backup = TrialDivision(n);
    factors.insert(factors.end(), backup.begin(), backup.end());
    return factors;
}

/**
 * Выводит результат факторизации на экран.
 * @param number Исходное число.
 * @param factors Вектор множителей.
 * @param duration Время выполнения в микросекундах.
 */
void PrintResult(int64_t number, const vector<int64_t>& factors, microseconds duration)
{
    wcout << L"Число " << number << L" = ";
    for (size_t i = 0; i < factors.size(); ++i)
    {
        if (i > 0) wcout << L" * ";
        wcout << factors[i];
    }
    wcout << L"\nВремя выполнения: " << duration.count() << L" мкс\n";
}

/**
 * Выводит справочную информацию о программе.
 */
void PrintHelp()
{
    wcout << L"Использование: factorize.exe --mode <режим> --number <число> [--help]\n"
        << L"  --mode trial_division   Метод перебора делителей\n"
        << L"  --mode fermats_method   Метод факторизации Ферма\n"
        << L"  --number n              Число для факторизации (целое >= 2)\n"
        << L"  --help                   Показать эту справку\n"
        << L"Пример: factorize.exe --mode trial_division --number 1234567890\n";
}

/**
 * Безопасное преобразование широкой строки в int64_t с проверкой ошибок.
 * @param str Входная строка.
 * @param value Ссылка для сохранения результата.
 * @return true если преобразование успешно и число >= 2.
 */
bool ParseInt64(const wstring& str, int64_t& value)
{
    if (str.empty())
    {
        wcerr << L"Ошибка: ввод не может быть пустым.\n";
        return false;
    }

    wchar_t* end;
    errno = 0;
    int64_t result = wcstoll(str.c_str(), &end, 10);

    if (errno == ERANGE || result > INT64_MAX)
    {
        wcerr << L"Ошибка: число слишком большое (выход за пределы 64-битного целого).\n";
        return false;
    }

    if (*end != L'\0')
    {
        wcerr << L"Ошибка: введено не целое число (содержит недопустимые символы).\n";
        return false;
    }

    if (result < 2)
    {
        wcerr << L"Ошибка: число должно быть >= 2.\n";
        return false;
    }

    value = result;
    return true;
}

/**
 * Разбирает аргументы командной строки и запускает соответствующий режим.
 * @param argc Количество аргументов.
 * @param argv Массив аргументов (широкие символы).
 */
void ProcessCommandLine(int argc, wchar_t* argv[])
{
    wstring mode;
    int64_t number = 0;
    bool helpRequested = false;

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
        else if (arg == L"--number" && i + 1 < argc)
        {
            if (!ParseInt64(argv[++i], number))
                return;
        }
    }

    if (helpRequested)
    {
        PrintHelp();
        return;
    }

    if (mode.empty())
    {
        wcerr << L"Ошибка: не указан режим --mode.\n";
        PrintHelp();
        return;
    }

    vector<int64_t> factors;
    high_resolution_clock::time_point start, end;
    microseconds duration;

    if (mode == L"trial_division")
    {
        start = high_resolution_clock::now();
        factors = TrialDivision(number);
        end = high_resolution_clock::now();
        duration = duration_cast<microseconds>(end - start);
        wcout << L"Метод перебора делителей:\n";
    }
    else if (mode == L"fermats_method")
    {
        start = high_resolution_clock::now();
        factors = FermatsMethod(number);
        end = high_resolution_clock::now();
        duration = duration_cast<microseconds>(end - start);
        wcout << L"Метод факторизации Ферма:\n";
    }
    else
    {
        wcerr << L"Ошибка: неизвестный режим. Допустимые: trial_division, fermats_method.\n";
        return;
    }

    PrintResult(number, factors, duration);
}

/**
 * Интерактивный режим работы.
 */
void InteractiveMode()
{
    wcout << L"Интерактивный режим факторизации. Для выхода введите пустую строку.\n";

    while (true)
    {
        wcout << L"\nВыберите действие:\n"
            << L"1. Перебор делителей (пробное деление)\n"
            << L"2. Метод факторизации Ферма\n"
            << L"3. Сравнить оба метода\n"
            << L"0. Выход\n"
            << L"Ваш выбор: ";

        wstring choice;
        getline(wcin, choice);

        if (choice == L"0")
            break;

        if (choice != L"1" && choice != L"2" && choice != L"3")
        {
            wcout << L"Неверный выбор. Попробуйте снова.\n";
            continue;
        }

        wcout << L"Введите число для факторизации (>= 2): ";
        wstring numStr;
        getline(wcin, numStr);
        if (numStr.empty())
        {
            wcout << L"Ошибка: ввод не может быть пустым.\n";
            continue;
        }

        int64_t number;
        if (!ParseInt64(numStr, number))
            continue;

        if (choice == L"1")
        {
            auto start = high_resolution_clock::now();
            vector<int64_t> factors = TrialDivision(number);
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            wcout << L"Метод перебора делителей:\n";
            PrintResult(number, factors, duration);
        }
        else if (choice == L"2")
        {
            auto start = high_resolution_clock::now();
            vector<int64_t> factors = FermatsMethod(number);
            auto end = high_resolution_clock::now();
            auto duration = duration_cast<microseconds>(end - start);
            wcout << L"Метод факторизации Ферма:\n";
            PrintResult(number, factors, duration);
        }
        else if (choice == L"3")
        {
            wcout << L"Сравнение методов для числа " << number << L":\n";

            auto start1 = high_resolution_clock::now();
            vector<int64_t> factors1 = TrialDivision(number);
            auto end1 = high_resolution_clock::now();
            auto dur1 = duration_cast<microseconds>(end1 - start1);

            auto start2 = high_resolution_clock::now();
            vector<int64_t> factors2 = FermatsMethod(number);
            auto end2 = high_resolution_clock::now();
            auto dur2 = duration_cast<microseconds>(end2 - start2);

            wcout << L"Перебор делителей: " << dur1.count() << L" мкс\n";
            PrintResult(number, factors1, dur1);
            wcout << L"Метод Ферма:       " << dur2.count() << L" мкс\n";
            PrintResult(number, factors2, dur2);

            vector<int64_t> sorted1 = factors1;
            vector<int64_t> sorted2 = factors2;
            sort(sorted1.begin(), sorted1.end());
            sort(sorted2.begin(), sorted2.end());

            if (sorted1 == sorted2)
                wcout << L"Результаты совпадают (множители одинаковы, порядок может отличаться).\n";
            else
                wcout << L"Внимание: результаты различаются! (ошибка в реализации)\n";
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

    if (argc > 1)
        ProcessCommandLine(argc, argv);
    else
        InteractiveMode();

    return 0;
}