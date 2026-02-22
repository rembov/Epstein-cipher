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

const int64_t SIEVE_LIMIT = 100000000;

/**
 * Оптимизированное решето Эратосфена (только нечётные)
 * @param n верхняя граница включительно (n >= 2)
 * @return вектор простых чисел
 */
vector<int64_t> SieveOfEratosthenes(int64_t n)
{
    if (n < 2) return {};
    vector<int64_t> primes;
    primes.push_back(2);
    if (n == 2) return primes;

    int64_t half = (n - 1) / 2;
    vector<bool> isPrime(half + 1, true);
    int64_t limit = static_cast<int64_t>(sqrt(n)) / 2;

    for (int64_t i = 1; i <= limit; ++i)
    {
        if (isPrime[i])
        {
            int64_t p = 2 * i + 1;
            int64_t step = p;
            int64_t start = (p * p - 1) / 2;
            for (int64_t j = start; j <= half; j += step)
                isPrime[j] = false;
        }
    }

    for (int64_t i = 1; i <= half; ++i)
        if (isPrime[i])
            primes.push_back(2 * i + 1);

    return primes;
}

/**
 * Возвращает вектор всех собственных делителей числа n (включая 1, исключая само n)
 * @param n число (>=2)
 * @return вектор делителей (отсортированный)
 */
vector<int64_t> GetProperDivisors(int64_t n)
{
    if (n < 2) return {};
    vector<int64_t> divisors;
    divisors.push_back(1);
    int64_t limit = static_cast<int64_t>(sqrt(n));
    for (int64_t d = 2; d <= limit; ++d)
    {
        if (n % d == 0)
        {
            divisors.push_back(d);
            int64_t other = n / d;
            if (other != d)
                divisors.push_back(other);
        }
    }
    sort(divisors.begin(), divisors.end());
    return divisors;
}

/**
 * Проверка числа на совершенность (сумма собственных делителей равна числу)
 * @param n проверяемое число (n >= 2)
 * @return true если число совершенное
 */
bool IsPerfectNumber(int64_t n)
{
    if (n < 2) return false;
    int64_t sum = 1;
    int64_t limit = static_cast<int64_t>(sqrt(n));
    for (int64_t d = 2; d <= limit; ++d)
    {
        if (n % d == 0)
        {
            sum += d;
            int64_t other = n / d;
            if (other != d)
                sum += other;
            if (sum > n) return false;
        }
    }
    return (n != 1) && (sum == n);
}

/**
 * Вывод результата решета Эратосфена
 */
void PrintSieveResult(int64_t limit, const vector<int64_t>& primes, microseconds duration)
{
    wcout << L"Простые числа до " << limit << L":\n";
    int count = 0;
    for (int64_t p : primes)
    {
        wcout << p << L" ";
        if (++count % 20 == 0) wcout << L"\n";
    }
    if (primes.size() % 20 != 0) wcout << L"\n";
    wcout << L"Всего простых чисел: " << primes.size() << L"\n";
    wcout << L"Время выполнения: " << duration.count() << L" мкс\n";
}

/**
 * Вывод результата проверки совершенного числа с деталями
 */
void PrintPerfectResult(int64_t n, bool isPerfect, microseconds duration)
{
    vector<int64_t> divisors = GetProperDivisors(n);
    int64_t sum = 0;
    for (int64_t d : divisors) sum += d;

    wcout << L"Число " << n << L"\n";
    wcout << L"Собственные делители: ";
    for (size_t i = 0; i < divisors.size(); ++i)
    {
        if (i > 0) wcout << L", ";
        wcout << divisors[i];
    }
    wcout << L"\nСумма делителей: " << sum << L"\n";
    wcout << L"Результат: " << (isPerfect ? L"является совершенным" : L"не является совершенным") << L"\n";
    wcout << L"Время выполнения: " << duration.count() << L" мкс\n";
}

/**
 * Справка
 */
void PrintHelp()
{
    wcout << L"Использование: primes.exe --mode <режим> --number <число> [--help]\n"
        << L"  --mode sieve    Решето Эратосфена (все простые до числа)\n"
        << L"  --mode perfect  Проверка числа на совершенность\n"
        << L"  --number n      Число (>=2, для sieve не более " << SIEVE_LIMIT << L")\n"
        << L"  --help          Справка\n"
        << L"Пример: primes.exe --mode sieve --number 100\n";
}

/**
 * Безопасное преобразование строки в int64_t
 */
bool ParseInt64(const wstring& str, int64_t& value)
{
    if (str.empty())
    {
        wcerr << L"Ошибка: пустой ввод\n";
        return false;
    }

    wchar_t* end;
    errno = 0;
    int64_t result = wcstoll(str.c_str(), &end, 10);

    if (errno == ERANGE || result > INT64_MAX)
    {
        wcerr << L"Ошибка: число слишком большое\n";
        return false;
    }

    if (*end != L'\0')
    {
        wcerr << L"Ошибка: недопустимые символы в числе\n";
        return false;
    }

    if (result < 2)
    {
        wcerr << L"Ошибка: число должно быть >= 2\n";
        return false;
    }

    value = result;
    return true;
}

/**
 * Проверка числа для режима sieve
 */
bool CheckSieveNumber(int64_t n)
{
    if (n > SIEVE_LIMIT)
    {
        wcerr << L"Ошибка: число превышает лимит " << SIEVE_LIMIT << L"\n";
        return false;
    }
    return true;
}

/**
 * Режим командной строки
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
        else
        {
            wcerr << L"Ошибка: неизвестный аргумент " << arg << L"\n";
            PrintHelp();
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
        wcerr << L"Ошибка: не указан режим --mode\n";
        PrintHelp();
        return;
    }

    if (number == 0)
    {
        wcerr << L"Ошибка: не указано число --number\n";
        PrintHelp();
        return;
    }

    if (mode == L"sieve")
    {
        if (!CheckSieveNumber(number))
            return;
        auto start = high_resolution_clock::now();
        vector<int64_t> primes = SieveOfEratosthenes(number);
        auto end = high_resolution_clock::now();
        PrintSieveResult(number, primes, duration_cast<microseconds>(end - start));
    }
    else if (mode == L"perfect")
    {
        auto start = high_resolution_clock::now();
        bool isPerfect = IsPerfectNumber(number);
        auto end = high_resolution_clock::now();
        PrintPerfectResult(number, isPerfect, duration_cast<microseconds>(end - start));
    }
    else
    {
        wcerr << L"Ошибка: неизвестный режим. Допустимые: sieve, perfect\n";
        PrintHelp();
    }
}

/**
 * Интерактивный режим
 */
void InteractiveMode()
{
    wcout << L"Выберите действие:\n"
        << L"1. Решето Эратосфена\n"
        << L"2. Проверка на совершенность\n"
        << L"3. Сравнение\n"
        << L"0. Выход\n";

    while (true)
    {
        wcout << L"> ";
        wstring choice;
        getline(wcin, choice);

        if (choice == L"0") break;
        if (choice != L"1" && choice != L"2" && choice != L"3")
        {
            wcout << L"Неверный выбор. Попробуйте снова.\n";
            continue;
        }

        wcout << L"Число: ";
        wstring numStr;
        getline(wcin, numStr);
        if (numStr.empty())
        {
            wcout << L"Ошибка: пустой ввод\n";
            continue;
        }

        int64_t number;
        if (!ParseInt64(numStr, number))
            continue;

        if (choice == L"1")
        {
            if (!CheckSieveNumber(number))
                continue;
            auto start = high_resolution_clock::now();
            vector<int64_t> primes = SieveOfEratosthenes(number);
            auto end = high_resolution_clock::now();
            PrintSieveResult(number, primes, duration_cast<microseconds>(end - start));
        }
        else if (choice == L"2")
        {
            auto start = high_resolution_clock::now();
            bool isPerfect = IsPerfectNumber(number);
            auto end = high_resolution_clock::now();
            PrintPerfectResult(number, isPerfect, duration_cast<microseconds>(end - start));
        }
        else
        {
            if (!CheckSieveNumber(number))
                continue;
            auto start1 = high_resolution_clock::now();
            vector<int64_t> primes = SieveOfEratosthenes(number);
            auto end1 = high_resolution_clock::now();
            auto dur1 = duration_cast<microseconds>(end1 - start1);

            auto start2 = high_resolution_clock::now();
            bool isPerfect = IsPerfectNumber(number);
            auto end2 = high_resolution_clock::now();
            auto dur2 = duration_cast<microseconds>(end2 - start2);

            wcout << L"--- Решето Эратосфена ---\n";
            PrintSieveResult(number, primes, dur1);
            wcout << L"--- Проверка на совершенность ---\n";
            PrintPerfectResult(number, isPerfect, dur2);
        }
    }
}

/**
 * Точка входа
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