#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <cstdlib>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#include <cerrno>
#include <algorithm>
#include <cstdint>

using namespace std;

/**
 * Проверка умножения на переполнение.
 * Если a * b > INT64_MAX, возвращает false, иначе true и сохраняет результат в result.
 */
bool SafeMultiply(int64_t a, int64_t b, int64_t& result)
{
    if (a == 0 || b == 0)
    {
        result = 0;
        return true;
    }
    if (a > INT64_MAX / b || a < INT64_MIN / b)
        return false;
    result = a * b;
    return true;
}

/**
 * Вычисление функции Эйлера φ(n) для заданного n.
 * @param n натуральное число (>=1)
 * @return значение φ(n)
 */
int64_t EulerPhi(int64_t n)
{
    if (n < 1) return 0;
    int64_t result = n;
    int64_t temp = n;
    for (int64_t p = 2; p * p <= temp; ++p)
    {
        if (temp % p == 0)
        {
            while (temp % p == 0)
                temp /= p;
            result -= result / p;
        }
    }
    if (temp > 1)
        result -= result / temp;
    return result;
}

/**
 * Расширенный алгоритм Евклида: находит d = gcd(a,b) и коэффициенты x,y такие, что a*x + b*y = d.
 * @param a, b целые числа (неотрицательные)
 * @param d ссылка для НОД
 * @param x ссылка для коэффициента при a
 * @param y ссылка для коэффициента при b
 */
void ExtendedGcd(int64_t a, int64_t b, int64_t& d, int64_t& x, int64_t& y)
{
    if (b == 0)
    {
        d = a;
        x = 1;
        y = 0;
    }
    else
    {
        ExtendedGcd(b, a % b, d, y, x);
        y -= (a / b) * x;
    }
}

/**
 * Решение системы сравнений x ≡ a[i] (mod n[i]) по китайской теореме об остатках.
 * Предполагается, что модули n[i] попарно взаимно просты и их произведение не переполняет int64_t.
 * @param a вектор остатков
 * @param n вектор модулей
 * @param result ссылка для сохранения результата
 * @return true при успехе, false при ошибке (переполнение)
 */
bool ChineseRemainder(const vector<int64_t>& a, const vector<int64_t>& n, int64_t& result)
{
    int64_t N = 1;
    for (int64_t mod : n)
    {
        int64_t newN;
        if (!SafeMultiply(N, mod, newN))
            return false;
        N = newN;
    }

    int64_t x = 0;
    for (size_t i = 0; i < a.size(); ++i)
    {
        int64_t Ni = N / n[i];
        int64_t d, inv, y;
        ExtendedGcd(Ni, n[i], d, inv, y);
        if (d != 1)
            return false; // не взаимно просты
        inv = (inv % n[i] + n[i]) % n[i];
        int64_t term1, term2;
        if (!SafeMultiply(a[i], Ni, term1))
            return false;
        if (!SafeMultiply(term1, inv, term2))
            return false;
        x = (x + term2) % N;
    }
    result = x;
    return true;
}

/**
 * Вывод результата для функции Эйлера.
 */
void PrintEulerResult(int64_t n, int64_t phi)
{
    wcout << L"φ(" << n << L") = " << phi << L"\n";
}

/**
 * Вывод результата для расширенного алгоритма Евклида.
 */
void PrintGcdResult(int64_t a, int64_t b, int64_t d, int64_t x, int64_t y)
{
    wcout << L"НОД(" << a << L", " << b << L") = " << d << L"\n";
    wcout << L"Коэффициенты: x = " << x << L", y = " << y << L"\n";
    wcout << a << L" * (" << x << L") + " << b << L" * (" << y << L") = " << d << L"\n";
}

/**
 * Вывод результата для китайской теоремы.
 */
void PrintCrtResult(const vector<int64_t>& a, const vector<int64_t>& n, int64_t x, int64_t N)
{
    wcout << L"Решение системы:\n";
    for (size_t i = 0; i < a.size(); ++i)
        wcout << L"x ≡ " << a[i] << L" (mod " << n[i] << L")\n";
    wcout << L"x = " << x << L" (mod " << N << L")\n";
}

/**
 * Справка.
 */
void PrintHelp()
{
    wcout << L"Использование: crypto.exe --mode <режим> [--number n] [--a a] [--b b] [--help]\n"
        << L"  --mode euler        Вычисление функции Эйлера (требуется --number)\n"
        << L"  --mode gcd          Расширенный алгоритм Евклида (требуется --a и --b)\n"
        << L"  --mode crt          Китайская теорема об остатках (ввод в интерактиве)\n"
        << L"  --number n          Число для функции Эйлера\n"
        << L"  --a a               Первое число для gcd\n"
        << L"  --b b               Второе число для gcd\n"
        << L"  --help              Справка\n"
        << L"Пример: crypto.exe --mode euler --number 100\n";
}

/**
 * Безопасное преобразование строки в int64_t.
 */
bool ParseInt64(const wstring& str, int64_t& value, bool positiveOnly = false)
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

    if (positiveOnly && result < 1)
    {
        wcerr << L"Ошибка: число должно быть положительным\n";
        return false;
    }

    value = result;
    return true;
}

/**
 * Режим командной строки.
 */
void ProcessCommandLine(int argc, wchar_t* argv[])
{
    wstring mode;
    int64_t number = 0, a = 0, b = 0;
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
            if (!ParseInt64(argv[++i], number, true))
                return;
        }
        else if (arg == L"--a" && i + 1 < argc)
        {
            if (!ParseInt64(argv[++i], a))
                return;
        }
        else if (arg == L"--b" && i + 1 < argc)
        {
            if (!ParseInt64(argv[++i], b))
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

    if (mode == L"euler")
    {
        if (number == 0)
        {
            wcerr << L"Ошибка: для режима euler требуется --number\n";
            return;
        }
        int64_t phi = EulerPhi(number);
        PrintEulerResult(number, phi);
    }
    else if (mode == L"gcd")
    {
        if (a == 0 || b == 0)
        {
            wcerr << L"Ошибка: для режима gcd требуются --a и --b\n";
            return;
        }
        int64_t d, x, y;
        ExtendedGcd(a, b, d, x, y);
        PrintGcdResult(a, b, d, x, y);
    }
    else if (mode == L"crt")
    {
        wcout << L"В режиме CRT данные вводятся интерактивно.\n";
        wcout << L"Введите количество уравнений: ";
        wstring line;
        getline(wcin, line);
        int k;
        try
        {
            k = stoi(line);
        }
        catch (...)
        {
            wcerr << L"Ошибка: неверное число\n";
            return;
        }
        if (k < 1)
        {
            wcerr << L"Ошибка: количество уравнений должно быть >= 1\n";
            return;
        }
        vector<int64_t> a(k), n(k);
        for (int i = 0; i < k; ++i)
        {
            wcout << L"Уравнение " << i + 1 << L": введите a и n через пробел: ";
            getline(wcin, line);
            size_t pos = line.find(L' ');
            if (pos == wstring::npos)
            {
                wcerr << L"Ошибка: ожидалось два числа\n";
                return;
            }
            wstring a_str = line.substr(0, pos);
            wstring n_str = line.substr(pos + 1);
            if (!ParseInt64(a_str, a[i]) || !ParseInt64(n_str, n[i], true))
                return;
        }
        for (int i = 0; i < k; ++i)
            for (int j = i + 1; j < k; ++j)
            {
                int64_t d, x, y;
                ExtendedGcd(n[i], n[j], d, x, y);
                if (d != 1)
                {
                    wcerr << L"Ошибка: модули " << n[i] << L" и " << n[j] << L" не взаимно просты\n";
                    return;
                }
            }
        int64_t result, N = 1;
        for (int64_t mod : n)
        {
            int64_t newN;
            if (!SafeMultiply(N, mod, newN))
            {
                wcerr << L"Ошибка: произведение модулей слишком велико (переполнение)\n";
                return;
            }
            N = newN;
        }
        if (!ChineseRemainder(a, n, result))
        {
            wcerr << L"Ошибка при решении системы (возможно, переполнение)\n";
            return;
        }
        PrintCrtResult(a, n, result, N);
    }
    else
    {
        wcerr << L"Ошибка: неизвестный режим. Допустимые: euler, gcd, crt\n";
        PrintHelp();
    }
}

/**
 * Интерактивный режим.
 */
void InteractiveMode()
{
    wcout << L"Выберите действие:\n"
        << L"1. Функция Эйлера\n"
        << L"2. Расширенный алгоритм Евклида\n"
        << L"3. Китайская теорема об остатках\n"
        << L"0. Выход\n";

    while (true)
    {
        wcout << L"> ";
        wstring choice;
        getline(wcin, choice);

        if (choice == L"0") break;

        if (choice == L"1")
        {
            wcout << L"Введите число n: ";
            wstring numStr;
            getline(wcin, numStr);
            int64_t n;
            if (!ParseInt64(numStr, n, true)) continue;
            int64_t phi = EulerPhi(n);
            PrintEulerResult(n, phi);
        }
        else if (choice == L"2")
        {
            wcout << L"Введите два числа a и b через пробел: ";
            wstring line;
            getline(wcin, line);
            size_t pos = line.find(L' ');
            if (pos == wstring::npos)
            {
                wcerr << L"Ошибка: ожидалось два числа\n";
                continue;
            }
            wstring a_str = line.substr(0, pos);
            wstring b_str = line.substr(pos + 1);
            int64_t a, b;
            if (!ParseInt64(a_str, a) || !ParseInt64(b_str, b))
                continue;
            int64_t d, x, y;
            ExtendedGcd(a, b, d, x, y);
            PrintGcdResult(a, b, d, x, y);
        }
        else if (choice == L"3")
        {
            wcout << L"Введите количество уравнений: ";
            wstring line;
            getline(wcin, line);
            int k;
            try
            {
                k = stoi(line);
            }
            catch (...)
            {
                wcerr << L"Ошибка: неверное число\n";
                continue;
            }
            if (k < 1)
            {
                wcerr << L"Ошибка: количество уравнений должно быть >= 1\n";
                continue;
            }
            vector<int64_t> a(k), n(k);
            bool ok = true;
            for (int i = 0; i < k && ok; ++i)
            {
                wcout << L"Уравнение " << i + 1 << L": введите a и n через пробел: ";
                getline(wcin, line);
                size_t pos = line.find(L' ');
                if (pos == wstring::npos)
                {
                    wcerr << L"Ошибка: ожидалось два числа\n";
                    ok = false;
                    break;
                }
                wstring a_str = line.substr(0, pos);
                wstring n_str = line.substr(pos + 1);
                if (!ParseInt64(a_str, a[i]) || !ParseInt64(n_str, n[i], true))
                {
                    ok = false;
                    break;
                }
            }
            if (!ok) continue;

            for (int i = 0; i < k; ++i)
                for (int j = i + 1; j < k; ++j)
                {
                    int64_t d, x, y;
                    ExtendedGcd(n[i], n[j], d, x, y);
                    if (d != 1)
                    {
                        wcerr << L"Ошибка: модули " << n[i] << L" и " << n[j] << L" не взаимно просты\n";
                        ok = false;
                        break;
                    }
                    if (!ok) break;
                }
            if (!ok) continue;

            int64_t N = 1;
            for (int64_t mod : n)
            {
                int64_t newN;
                if (!SafeMultiply(N, mod, newN))
                {
                    wcerr << L"Ошибка: произведение модулей слишком велико (переполнение)\n";
                    ok = false;
                    break;
                }
                N = newN;
            }
            if (!ok) continue;

            int64_t result;
            if (!ChineseRemainder(a, n, result))
            {
                wcerr << L"Ошибка при решении системы (возможно, переполнение)\n";
                continue;
            }
            PrintCrtResult(a, n, result, N);
        }
        else
        {
            wcout << L"Неверный выбор. Попробуйте снова.\n";
        }
    }
}

/**
 * Точка входа.
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