#include <iostream>
#include <chrono>
#include <ctime>
#include <cstring> // 用于字符串操作
const int MAX_MESSAGE_LEN=1024;

struct ChatMessage {
    char username[20];  // 用户名
    char message[MAX_MESSAGE_LEN];   // 用户发送的消息
    int roomId;                      // 用户所在的房间号，暂时先不用管
    char timestamp[20]; // 添加时间戳成员

    ChatMessage() {}

    // 构造函数，用于方便初始化 ChatMessage 的实例
    ChatMessage(const char* _username, const char* _message, int _roomId)
        : roomId(_roomId) {
        // 复制用户名和消息
        strncpy(username, _username, MAX_MESSAGE_LEN - 1);
        username[MAX_MESSAGE_LEN - 1] = '\0'; // 确保字符串以 null 结尾

        strncpy(message, _message, MAX_MESSAGE_LEN - 1);
        message[MAX_MESSAGE_LEN - 1] = '\0'; // 确保字符串以 null 结尾

        // 获取当前时间，并将其格式化为字符串
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&time_t_now));
    }
};
void PrintChatMessage(const ChatMessage& message) {
    std::cout << "Room ID: " << message.roomId << std::endl;
    std::cout << "Username: " << message.username << std::endl;
    std::cout << "Message: " << message.message << std::endl;
    std::cout << "Timestamp: " << message.timestamp << std::endl;
}

void printTimeInfo() {
    // 获取当前时间日期信息
    time_t now = time(nullptr); // 用了 ctime
    tm localTime;
    localtime_s(&localTime, &now);
    
    // 输出当前时间日期信息
    std::cout << (localTime.tm_year + 1900) << "."
              << (localTime.tm_mon + 1) << "."
              << localTime.tm_mday << " "
              << localTime.tm_hour << ":"
              << localTime.tm_min << ":"
              << localTime.tm_sec << "" << std::endl;
}

// 更新时间戳的函数
void UpdateTimestamp(ChatMessage& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    strftime(message.timestamp, sizeof(message.timestamp), "%Y-%m-%d %H:%M:%S", localtime(&time_t_now));
}