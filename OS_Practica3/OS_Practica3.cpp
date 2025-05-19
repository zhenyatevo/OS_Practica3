#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

const char* FILE_NAME = "chat_log.txt";

class ChatSync {
private:
    mutex mtx;
    condition_variable cv;
    bool chat1_turn = true;
    atomic<bool> running{ true };

public:
    void wait_for_turn(int chat_id) {
        unique_lock<mutex> lock(mtx);
        cv.wait(lock, [this, chat_id] {
            return !running || (chat_id == 1 && chat1_turn) || (chat_id == 2 && !chat1_turn);
            });
    }

    void switch_turn() {
        lock_guard<mutex> lock(mtx);
        chat1_turn = !chat1_turn;
        cv.notify_all();
    }

    void stop() {
        lock_guard<mutex> lock(mtx);
        running = false;
        cv.notify_all();
    }

    bool is_running() const {
        return running;
    }

    bool is_chat1_turn() const {
        return chat1_turn;
    }
};

void chat_session(int chat_id, ChatSync& sync) {
    while (sync.is_running()) {
        sync.wait_for_turn(chat_id);
        if (!sync.is_running()) break;



        // Режим чтения сообщения
        ifstream file(FILE_NAME);
        if (file.is_open()) {
            string line;
            while (getline(file, line)) {
                size_t colon_pos = line.find(":");
                if (colon_pos != string::npos) {
                    string sender = line.substr(0, colon_pos);
                    string message_content = line.substr(colon_pos + 2);
                    cout << "Chat" << chat_id << " read " << sender << ": \"" << message_content << "\"" << endl;
                }
            }
            file.close();
        }

        if ((chat_id == 1 && sync.is_chat1_turn()) || (chat_id == 2 && !sync.is_chat1_turn())) {
            // Режим отправки сообщения
            string message;
            cout << "Chat" << chat_id << ": ";
            getline(cin, message);

            if (message == "exit") {
                sync.stop();
                break;
            }

            ofstream file(FILE_NAME, ios::trunc);
            if (file.is_open()) {
                file << "Chat" << chat_id << ": " << message;
                file.close();
            }
        }
        else {
            
        }

        sync.switch_turn();
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    cout << "=== Программа поочередного чата ===" << endl;
    cout << "Чаты будут по очереди отправлять и получать сообщения" << endl;
    cout << "Для выхода введите 'exit'" << endl << endl;

    // Очищаем файл при старте
    ofstream clear_file(FILE_NAME, ios::trunc);
    clear_file.close();

    ChatSync sync;
    thread chat1(chat_session, 1, ref(sync));
    thread chat2(chat_session, 2, ref(sync));

    chat1.join();
    chat2.join();

    cout << "Программа завершена." << endl;
    return 0;
}