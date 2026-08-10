#include <iostream>
#include <string>
#include <vector>
#include <regex>
#include <curl/curl.h>
#include <fstream>
#include <windows.h>

using namespace std;

struct SearchResult {
    string title;
    string url;
    string content;
    string engine;
};

size_t writeCallback(void* contents, size_t size, size_t nmemb, string* userData) {
    userData->append((char*)contents, size * nmemb);
    return size * nmemb;
}

string urlEncode(const string& value) {
    string encoded;
    char hex[] = "0123456789ABCDEF";
    for (unsigned char c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded += c;
        } else {
            encoded += '%';
            encoded += hex[(c >> 4) & 0xF];
            encoded += hex[c & 0xF];
        }
    }
    return encoded;
}

// 转义 JSON 字符串中的特殊字符
string jsonEscape(const string& s) {
    string result;
    for (char c : s) {
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

vector<SearchResult> searchSogou(const string& query) {
    vector<SearchResult> results;
    
    CURL* curl = curl_easy_init();
    if (!curl) return results;
    
    string encodedQuery = urlEncode(query);
    string url = "https://www.sogou.com/web?query=" + encodedQuery;
    
    string response;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, 
        "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/126.0.0.0 Safari/537.36");
    
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
    headers = curl_slist_append(headers, "Accept-Language: zh-CN,zh;q=0.9");
    headers = curl_slist_append(headers, "Connection: keep-alive");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(curl);
    
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    ofstream debug("sogou_debug.html");
    debug << response;
    debug.close();
    
    if (res != CURLE_OK || httpCode != 200) return results;
    
    // 搜狗解析
    regex resultRegex("<div class=\"vrwrap\"[^>]*>.*?<h3.*?><a[^>]*href=\"([^\"]*)\"[^>]*>(.*?)</a></h3>.*?<p[^>]*class=\"str_info\"[^>]*>(.*?)</p>.*?</div>");
    
    smatch match;
    string::const_iterator searchStart(response.cbegin());
    
    while (regex_search(searchStart, response.cend(), match, resultRegex)) {
        SearchResult r;
        r.url = match[1];
        r.title = regex_replace(match[2].str(), regex("<[^>]+>"), "");
        r.content = regex_replace(match[3].str(), regex("<[^>]+>"), "");
        r.engine = "sogou";
        results.push_back(r);
        searchStart = match.suffix().first;
    }
    
    return results;
}

void outputJSON(const vector<SearchResult>& results, const string& query) {
    // 不设置控制台编码，直接输出
    cout << "{" << endl;
    cout << "  \"query\": \"" << jsonEscape(query) << "\"," << endl;
    cout << "  \"number_of_results\": " << results.size() << "," << endl;
    cout << "  \"results\": [" << endl;
    
    for (size_t i = 0; i < results.size(); i++) {
        cout << "    {" << endl;
        cout << "      \"title\": \"" << jsonEscape(results[i].title) << "\"," << endl;
        cout << "      \"url\": \"" << jsonEscape(results[i].url) << "\"," << endl;
        cout << "      \"content\": \"" << jsonEscape(results[i].content) << "\"," << endl;
        cout << "      \"engine\": \"" << results[i].engine << "\"" << endl;
        cout << "    }";
        if (i < results.size() - 1) cout << ",";
        cout << endl;
    }
    
    cout << "  ]" << endl;
    cout << "}" << endl;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        cerr << "Usage: " << argv[0] << " <search query>" << endl;
        return 1;
    }
    
    string query = argv[1];
    for (int i = 2; i < argc; i++) {
        query += " " + string(argv[i]);
    }
    
    vector<SearchResult> results = searchSogou(query);
    outputJSON(results, query);
    
    return 0;
}
