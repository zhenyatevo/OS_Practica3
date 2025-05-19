/*
Задание 3: написать общение через файл,т.е. два чата, используя семафор
*/

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <locale>
#include <codecvt>
#include <io.h>
#include <fcntl.h>

using namespace std;

// Имя файла для обмена сообщениями (в формате Unicode)
const wchar_t* FILE_NAME = L"chat_log.txt";

/*
Класс для синхронизации работы двух чатов
Обеспечивает поочередный доступ к файлу
*/
class ChatSync {
private:
    mutex mtx;                      // Мьютекс для защиты общих данных
    condition_variable cv;          // Условная переменная для ожидания очереди
    bool chat1_turn = true;         // Флаг очереди (true - очередь Chat1)
    atomic<bool> running{ true };     // Атомарный флаг работы программы

public:
    // Ожидание своей очереди для чата с указанным ID
    void wait_for_turn(int chat_id) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, chat_id] {
            return !running || (chat_id == 1 && chat1_turn) || (chat_id == 2 && !chat1_turn);
            });
    }

    // Переключение очереди между чатами
    void switch_turn() {
        lock_guard<mutex> lock(mtx);
        chat1_turn = !chat1_turn;
        cv.notify_all(); // Уведомляем все ожидающие потоки
    }

    // Остановка работы программы
    void stop() {
        lock_guard<mutex> lock(mtx);
        running = false;
        cv.notify_all();
    }

    // Проверка состояния работы
    bool is_running() const {
        return running;
    }

    // Проверка, очередь ли сейчас Chat1
    bool is_chat1_turn() const {
        return chat1_turn;
    }
};

// Настройка консоли для работы с Unicode (русские символы)
void init_console() {
    _setmode(_fileno(stdout), _O_U16TEXT);  // Unicode для вывода
    _setmode(_fileno(stdin), _O_U16TEXT);   // Unicode для ввода
    _setmode(_fileno(stderr), _O_U16TEXT);  // Unicode для ошибок
    setlocale(LC_ALL, "Russian");           // Локализация для Windows
}

// Запись широкой строки в файл (UTF-16)
void write_to_file(const wstring& content) {
    wofstream file(FILE_NAME, ios::binary | ios::trunc); // Открываем в бинарном режиме
    if (file.is_open()) {
        // Устанавливаем кодировку UTF-16 Little Endian
        file.imbue(locale(locale::empty(), new codecvt_utf16<wchar_t, 0x10ffff, little_endian>));
        file << content; // Записываем содержимое
        file.close();
    }
}

// Чтение широкой строки из файла (UTF-16)
wstring read_from_file() {
    wifstream file(FILE_NAME, ios::binary); // Открываем в бинарном режиме
    wstring content;
    if (file.is_open()) {
        // Устанавливаем кодировку UTF-16 Little Endian
        file.imbue(locale(locale::empty(), new codecvt_utf16<wchar_t, 0x10ffff, little_endian>));
        getline(file, content, L'\0'); // Читаем до нулевого символа
        file.close();
    }
    return content;
}

/*
Функция работы одного чата
chat_id - идентификатор чата (1 или 2)
sync - объект синхронизации
*/
void chat_session(int chat_id, ChatSync& sync) {
    while (sync.is_running()) {
        // Ожидаем своей очереди
        sync.wait_for_turn(chat_id);
        if (!sync.is_running()) break;

        // Чтение сообщения из файла
        wstring file_content = read_from_file();
        if (!file_content.empty()) {
            // Парсим сообщение: "ChatX: текст"
            size_t colon_pos = file_content.find(L':');
            if (colon_pos != wstring::npos) {
                wstring sender = file_content.substr(0, colon_pos);
                wstring message_content = file_content.substr(colon_pos + 2);
                // Выводим в формате: "ChatY read ChatX: "текст""
                wcout << L"Chat" << chat_id << L" read " << sender << L": \"" << message_content << L"\"" << endl;
            }
        }

        // Если сейчас наша очередь отправлять сообщение
        if ((chat_id == 1 && sync.is_chat1_turn()) || (chat_id == 2 && !sync.is_chat1_turn())) {
            wstring message;
            wcout << L"Chat" << chat_id << L": ";
            getline(wcin, message); // Читаем сообщение от пользователя

            if (message == L"exit") {
                sync.stop(); // Команда выхода
                break;
            }

            // Записываем сообщение в файл
            write_to_file(L"Chat" + to_wstring(chat_id) + L": " + message);
        }

        // Переключаем очередь
        sync.switch_turn();
        this_thread::sleep_for(chrono::milliseconds(100)); // Небольшая пауза
    }
}

int main() {
    // Инициализация консоли
    init_console();

    // Приветственное сообщение
    wcout << L"=== Программа поочередного чата ===" << endl;
    wcout << L"Чаты будут по очереди отправлять и получать сообщения" << endl;
    wcout << L"Для выхода введите 'exit'" << endl << endl;

    // Очищаем файл при старте
    write_to_file(L"");

    // Создаем объект синхронизации
    ChatSync sync;

    // Запускаем два чата в отдельных потоках
    thread chat1(chat_session, 1, ref(sync));
    thread chat2(chat_session, 2, ref(sync));

    // Ожидаем завершения потоков
    chat1.join();
    chat2.join();

    wcout << L"Программа завершена." << endl;
    return 0;
}
