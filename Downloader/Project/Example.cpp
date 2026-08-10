#include <iostream>
#include <string>
#include <curl/curl.h>
#include "json.hpp"

using json = nlohmann::json;

std::string Address = "https://api.iamhc.cn/v1/chat/completions";
std::string Key = "sk-ZEXD2e6QmckjYV3iaA8NWfRt7Sv6PmRWdVuTbQfQ7W7yiovz";

size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* userData) {
    userData->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string jsonEscape(const std::string& s) {
    std::string result;
    for (unsigned char c : s) {
        switch (c) {
            case '\"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 0x20) {
                    char buf[7];
                    sprintf(buf, "\\u%04x", c);
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

int main() {
    std::string userInput;
    std::cout << "你想对 AI 说什么: ";
    std::getline(std::cin, userInput);

    std::string escapedInput = jsonEscape(userInput);
    std::string jsonBody = "{\"model\":\"auto\",\"group\":\"default\",\"messages\":[{\"role\":\"user\",\"content\":\"" + escapedInput + "\"}]}";

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "CURL 初始化失败" << std::endl;
        return 1;
    }

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, Address.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonBody.c_str());

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + Key).c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);

    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    std::cout << "HTTP 状态码: " << httpCode << std::endl;

    if (res != CURLE_OK) {
        std::cerr << "请求失败: " << curl_easy_strerror(res) << std::endl;
    } else if (httpCode == 402) {
        std::cerr << "余额不足，请充值 DeepSeek API" << std::endl;
    } else {
        try {
            // 用 nlohmann/json 解析
            json j = json::parse(response);
            std::string reply = j["choices"][0]["message"]["content"];
            std::cout << "\n========== AI 回复 ==========\n" << reply << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "JSON 解析失败: " << e.what() << std::endl;
            std::cout << "原始响应:\n" << response << std::endl;
        }
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return 0;
}
