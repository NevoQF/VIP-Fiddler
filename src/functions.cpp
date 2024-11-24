#include "../headers/config.h"
#include "../headers/globals.h"
#include "../headers/funcs.h"
#include "../headers/tray.h"
#include <curl/curl.h>
#include <json/json.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <regex>
#include <Windows.h>
#include <random>
#include <unordered_map>
#include <objbase.h>
#include <sstream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <fstream>
#include <shellapi.h>
#include <thread>

std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

void handleKeybindPress(const std::string& KEYBIND) {
    int vkKeyCode = getVirtualKeyCode(KEYBIND);
    if (vkKeyCode == -1) {
        std::cout << "Invalid keybind: " << KEYBIND << "\n";
        return;
    }

    std::cout << "Press " << KEYBIND << " to join the server from your clipboard.\n";
    while (true) {
        if (GetAsyncKeyState(vkKeyCode) & 0x8000) {
            std::cout << KEYBIND << " pressed. Attempting to join server.\n";
            startTime = std::chrono::high_resolution_clock::now();
            std::string vipServerLink = getClipboardText();
            if (!vipServerLink.empty()) {
                joinPrivateServer(vipServerLink);
            }
            else {
                std::cout << "Clipboard empty or does not contain text.\n";
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

int getVirtualKeyCode(const std::string& keyName) {
    static std::unordered_map<std::string, int> keyMap = {
        {"F1", VK_F1},     {"F2", VK_F2},     {"F3", VK_F3},
        {"F4", VK_F4},     {"F5", VK_F5},     {"F6", VK_F6},
        {"F7", VK_F7},     {"F8", VK_F8},     {"F9", VK_F9},
        {"F10", VK_F10},   {"F11", VK_F11},   {"F12", VK_F12},
        {"A", 'A'},        {"B", 'B'},        {"C", 'C'},
        {"D", 'D'},        {"E", 'E'},        {"F", 'F'},
        {"G", 'G'},        {"H", 'H'},        {"I", 'I'},
        {"J", 'J'},        {"K", 'K'},        {"L", 'L'},
        {"M", 'M'},        {"N", 'N'},        {"O", 'O'},
        {"P", 'P'},        {"Q", 'Q'},        {"R", 'R'},
        {"S", 'S'},        {"T", 'T'},        {"U", 'U'},
        {"V", 'V'},        {"W", 'W'},        {"X", 'X'},
        {"Y", 'Y'},        {"Z", 'Z'},        {"0", '0'},
        {"1", '1'},        {"2", '2'},        {"3", '3'},
        {"4", '4'},        {"5", '5'},        {"6", '6'},
        {"7", '7'},        {"8", '8'},        {"9", '9'},
        {"ENTER", VK_RETURN}, {"ESC", VK_ESCAPE}, {"SPACE", VK_SPACE},
        {"TAB", VK_TAB},     {"BACKSPACE", VK_BACK}, {"DELETE", VK_DELETE},
        {"UP", VK_UP},       {"DOWN", VK_DOWN},     {"LEFT", VK_LEFT},
        {"RIGHT", VK_RIGHT}
    };

    auto it = keyMap.find(keyName);
    if (it != keyMap.end()) {
        return it->second;
    }
    else {
        return -1;
    }
}

std::pair<std::string, std::string> getUserIDAndUsernameFromCookie(const std::string& cookie) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;
    std::string userID;
    std::string username;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        std::string cookieHeader = ".ROBLOSECURITY=" + cookie;

        headers = curl_slist_append(headers, ("Cookie: " + cookieHeader).c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "https://users.roblox.com/v1/users/authenticated");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Roblox/WinInet");
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cout << "Failed to get user info: " << curl_easy_strerror(res) << "\n";
        }
        else {
            Json::CharReaderBuilder reader;
            Json::Value jsonData;
            std::string errs;
            std::istringstream iss(readBuffer);
            if (Json::parseFromStream(reader, iss, &jsonData, &errs)) {
                if (jsonData.isMember("id") && jsonData.isMember("name")) {
                    userID = jsonData["id"].asString();
                    username = jsonData["name"].asString();
                }
                else {
                    std::cout << "User info not found in response.\n";
                }
            }
            else {
            }
        }
        curl_easy_cleanup(curl);
        curl_slist_free_all(headers);
    }
    return { userID, username };
}

std::string fetchCsrfToken() {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    if (!csrfTokenCache.empty() && (now - csrfTokenLastFetched) < 10 * 60 * 1000) {
        std::cout << "Using cached CSRF token.\n";
        return csrfTokenCache;
    }
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Failed to initialize CURL.\n";
        return "";
    }
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + ROBLOSECURITY).c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, "https://auth.roblox.com/v2/logout");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cout << "CURL perform failed: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return "";
    }
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    if (response_code != 200 && response_code != 403) {
        std::cout << "Unexpected response code: " << response_code << "\n";
        std::cout << "Full Response:\n" << readBuffer << "\n";
    }
    std::string csrfToken;
    std::istringstream stream(readBuffer);
    std::string line;
    bool headersEnded = false;
    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r") {
            headersEnded = true;
            continue;
        }
        if (!headersEnded) {
            std::string headerName = "x-csrf-token:";
            if (line.size() >= headerName.size()) {
                std::string line_lower = line;
                std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(), ::tolower);
                std::string headerName_lower = "x-csrf-token:";
                if (line_lower.find(headerName_lower) == 0) {
                    csrfToken = line.substr(headerName.size());
                    csrfToken.erase(0, csrfToken.find_first_not_of(" \t\r\n"));
                    csrfToken.erase(csrfToken.find_last_not_of(" \t\r\n") + 1);
                    break;
                }
            }
        }
    }
    if (!csrfToken.empty()) {
        csrfTokenCache = csrfToken;
        csrfTokenLastFetched = now;
    }
    else {
        std::cout << "CSRF token not found in response headers.\n";
        std::cout << "Full Response:\n" << readBuffer << "\n";
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return csrfTokenCache;
}

std::string generateUUID() {
    GUID guid;
    HRESULT hCreateGuid = CoCreateGuid(&guid);
    if (hCreateGuid != S_OK) {
        std::cout << "Failed to generate GUID.\n";
        return "";
    }
    std::stringstream ss;
    ss << std::uppercase << std::hex << std::setfill('0');
    ss << std::setw(8) << guid.Data1 << "-";
    ss << std::setw(4) << guid.Data2 << "-";
    ss << std::setw(4) << guid.Data3 << "-";
    ss << std::setw(2) << static_cast<int>(guid.Data4[0]);
    ss << std::setw(2) << static_cast<int>(guid.Data4[1]) << "-";
    for (int i = 2; i < 8; ++i) {
        ss << std::setw(2) << static_cast<int>(guid.Data4[i]);
    }
    return ss.str();
}

void launchRobloxClient(const std::string& encodedUrl, const std::string& ticket, const std::string& browserTrackerID) {
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::string robloxUrl = "roblox-player:1+launchmode:play+gameinfo:" + ticket +
        "+launchtime:" + std::to_string(now) +
        "+placelauncherurl:" + encodedUrl +
        "+browsertrackerid:" + browserTrackerID +
        "+robloxLocale:en_us+gameLocale:en_us+channel:+LaunchExp:InApp";
    std::string command = "start \"\" \"" + robloxUrl + "\"";
    std::cout << "Joined server.\n";
    auto endTime = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsedTime = endTime - startTime;
    std::cout << "Took " << elapsedTime.count() << "s to join the server.\n";
    system(command.c_str());
}

std::string parseAccessCode(const std::string& htmlContent) {
    std::regex rgx1(R"(\bRoblox\.GameLauncher\.joinPrivateGame\(\d+,\s*'([\w-]+)')");
    std::regex rgx2(R"(\bRobloxLaunch\.RequestPrivateGame\(\s*'[^']*',\s*\d+,\s*'[\w-]+',\s*\bnull\b,\s*'([\w-]+)'\s*\))");
    std::regex rgx3(R"(\bRobloxLaunch\.RequestPrivateGame\(\s*'[^']*',\s*\d+,\s*'([\w-]+)',)");
    std::smatch match;

    if (std::regex_search(htmlContent, match, rgx1)) {
        return match[1].str();
    }
    else if (std::regex_search(htmlContent, match, rgx3)) {
        return match[1].str();
    }
    else if (std::regex_search(htmlContent, match, rgx2)) {
        return match[1].str();
    }
    else {
        std::cout << "No match found for accessCode.\n";
        return "";
    }
}

std::string getAuthTicket(const std::string& csrfToken) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Failed to initialize CURL for auth ticket.\n";
        return "";
    }
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-CSRF-TOKEN: " + csrfToken).c_str());
    headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + ROBLOSECURITY).c_str());
    headers = curl_slist_append(headers, "User-Agent: Roblox/WinInet");
    headers = curl_slist_append(headers, "Referer: https://www.roblox.com/home");
    curl_easy_setopt(curl, CURLOPT_URL, "https://auth.roblox.com/v1/authentication-ticket/");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_HEADER, 1L);
    CURLcode res = curl_easy_perform(curl);
    std::string ticket;
    if (res != CURLE_OK) {
        std::cout << "Error fetching auth ticket: " << curl_easy_strerror(res) << "\n";
    }
    else {
        long response_code;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200) {
            std::cout << "Unexpected response code for auth ticket: " << response_code << "\n";
        }
        std::string authTicket;
        std::istringstream stream(readBuffer);
        std::string line;
        bool headersEnded = false;
        while (std::getline(stream, line)) {
            if (line.empty() || line == "\r") {
                headersEnded = true;
                continue;
            }
            if (!headersEnded) {
                std::string headerName = "rbx-authentication-ticket:";
                std::string line_lower = line;
                std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(), ::tolower);
                std::string headerName_lower = "rbx-authentication-ticket:";
                if (line_lower.find(headerName_lower) == 0) {
                    authTicket = line.substr(headerName.size());
                    authTicket.erase(0, authTicket.find_first_not_of(" \t\r\n"));
                    authTicket.erase(authTicket.find_last_not_of(" \t\r\n") + 1);
                    break;
                }
            }
        }
        if (!authTicket.empty()) {
            ticket = authTicket;
        }
        else {
            std::cout << "Auth ticket not found in response headers.\n";
        }
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return ticket;
}

std::string getClipboardText() {
    if (!OpenClipboard(NULL)) {
        std::cout << "Failed to open clipboard.\n";
        return "";
    }
    HANDLE hData = GetClipboardData(CF_TEXT);
    if (hData == NULL) {
        std::cout << "No text data in clipboard.\n";
        CloseClipboard();
        return "";
    }
    char* pszText = static_cast<char*>(GlobalLock(hData));
    if (pszText == NULL) {
        std::cout << "Failed to lock clipboard data.\n";
        CloseClipboard();
        return "";
    }
    std::string clipboardText(pszText);
    GlobalUnlock(hData);
    CloseClipboard();
    return clipboardText;
}

std::string extractBody(const std::string& response) {
    size_t pos = response.find("\r\n\r\n");
    if (pos != std::string::npos) {
        return response.substr(pos + 4);
    }
    return "";
}

std::string getPlaceIdFromUrl(const std::string& link) {
    try {
        std::regex rgx(R"(https?://www\.roblox\.com(?:/\w+)?/games/(\d+).*)");
        std::smatch match;
        if (std::regex_search(link, match, rgx)) {
            return match[1];
        }
        else {
            return "";
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error parsing placeId: " << e.what() << "\n";
        return "";
    }
}

std::string getPrivateServerLinkCode(const std::string& link) {
    try {
        std::regex rgx(R"((?:\?|&)privateServerLinkCode=([\w-]+))");
        std::smatch match;
        if (std::regex_search(link, match, rgx)) {
            return match[1];
        }
        else {
            return "";
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error extracting privateServerLinkCode: " << e.what() << "\n";
        return "";
    }
}

struct ShareLinkData {
    std::string privateServerLinkCode;
    std::string placeId;
    std::string jobId;
    std::string referredByUserId;
};

std::string extractFirstJson(const std::string& response) {
    size_t start = response.find('{');
    if (start == std::string::npos) return "";
    int braceCount = 0;
    size_t end = start;
    for (; end < response.size(); ++end) {
        if (response[end] == '{') braceCount++;
        else if (response[end] == '}') braceCount--;
        if (braceCount == 0) break;
    }
    if (braceCount == 0 && end > start) {
        return response.substr(start, end - start + 1);
    }
    return "";
}

ShareLinkData resolveShareLinkCode(const std::string& shareCode, const std::string& shareType, const std::string& csrfToken) {
    CURL* curl = curl_easy_init();
    ShareLinkData data;
    if (!curl) return data;

    std::string readBuffer;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
    headers = curl_slist_append(headers, ("X-CSRF-TOKEN: " + csrfToken).c_str());
    headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + ROBLOSECURITY).c_str());

    Json::Value jsonBody;
    jsonBody["linkId"] = shareCode;
    jsonBody["linkType"] = shareType;
    Json::StreamWriterBuilder writer;
    std::string requestBody = Json::writeString(writer, jsonBody);

    curl_easy_setopt(curl, CURLOPT_URL, "https://apis.roblox.com/sharelinks/v1/resolve-link");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cout << "CURL perform failed: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return data;
    }

    std::string body = extractFirstJson(readBuffer);
    Json::Value jsonData;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream ss(body);
    if (Json::parseFromStream(readerBuilder, ss, &jsonData, &errs)) {
        if (shareType == "Server" && jsonData.isMember("privateServerInviteData") && jsonData["privateServerInviteData"]["status"].asString() == "Valid") {
            data.privateServerLinkCode = jsonData["privateServerInviteData"]["linkCode"].asString();
            data.placeId = std::to_string(jsonData["privateServerInviteData"]["placeId"].asUInt64());
        }
        else if (shareType == "ExperienceInvite" && jsonData.isMember("experienceInviteData") && jsonData["experienceInviteData"]["status"].asString() == "Valid") {
            data.placeId = std::to_string(jsonData["experienceInviteData"]["placeId"].asUInt64());

            data.jobId = jsonData["experienceInviteData"]["instanceId"].asString();

            data.referredByUserId = std::to_string(jsonData["experienceInviteData"]["inviterId"].asUInt64());
        }
        else {
            std::cout << "Invalid or unsupported shareType or data status is not 'Valid'.\n";
        }
    }
    else {
        std::cout << "Failed to parse JSON response: " << errs << "\n";
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return data;
}

std::string retrieveAccessCode(const std::string& placeId, const std::string& linkCode, const std::string& csrfToken) {
    CURL* curl = curl_easy_init();
    if (!curl) return "";
    std::string readBuffer;
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, ("X-CSRF-TOKEN: " + csrfToken).c_str());
    headers = curl_slist_append(headers, ("Cookie: .ROBLOSECURITY=" + ROBLOSECURITY).c_str());
    headers = curl_slist_append(headers, "User-Agent: Roblox/WinInet");
    headers = curl_slist_append(headers, "Referer: https://www.roblox.com/home");
    headers = curl_slist_append(headers, "Accept-Language: en-US,en;q=0.9");
    std::string url = "https://www.roblox.com/games/" + placeId + "?privateServerLinkCode=" + linkCode;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cout << "Error fetching access code: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return "";
    }
    long response_code;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    if (response_code == 200) {
        return parseAccessCode(readBuffer);
    }
    else {
        std::cout << "Failed to retrieve access code: HTTP " << response_code << "\n";
        std::cout << "Full Response:\n" << readBuffer << "\n";
        return "";
    }
}

std::string getUserIdFromProfileUrl(const std::string& link) {
    try {
        std::regex rgx(R"(https?://www\.roblox\.com/users/(\d+)/profile)");
        std::smatch match;
        if (std::regex_search(link, match, rgx)) {
            return match[1];
        }
        else {
            std::regex rgxWithExtra(R"(https?://www\.roblox\.com/users/(\d+)/profile.*)");
            if (std::regex_search(link, match, rgxWithExtra)) {
                return match[1];
            }
        }
    }
    catch (const std::exception& e) {
        std::cout << "Error parsing user ID from profile URL: " << e.what() << "\n";
    }
    return "";
}

std::string getUserIdFromUsername(const std::string& username) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cout << "Failed to initialize CURL for getting user ID from username.\n";
        return "";
    }
    std::string readBuffer;
    std::string url = "https://users.roblox.com/v1/usernames/users";
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    Json::Value jsonBody;
    Json::Value usernames(Json::arrayValue);
    usernames.append(username);
    jsonBody["usernames"] = usernames;
    jsonBody["excludeBannedUsers"] = false;
    Json::StreamWriterBuilder writer;
    std::string requestBody = Json::writeString(writer, jsonBody);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, requestBody.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        std::cout << "Error fetching user ID from username: " << curl_easy_strerror(res) << "\n";
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return "";
    }

    Json::Value jsonData;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream ss(readBuffer);
    if (Json::parseFromStream(readerBuilder, ss, &jsonData, &errs)) {
        if (jsonData.isMember("data") && jsonData["data"].size() > 0 && jsonData["data"][0].isMember("id")) {
            std::string userId = std::to_string(jsonData["data"][0]["id"].asUInt64());
            curl_slist_free_all(headers);
            curl_easy_cleanup(curl);
            return userId;
        }
        else {
            std::cout << "User not found or invalid response.\n";
        }
    }
    else {
        std::cout << "Failed to parse JSON response: " << errs << "\n";
    }
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return "";
}

void followUser(const std::string& userId) {
    std::string csrfToken = fetchCsrfToken();
    if (csrfToken.empty()) {
        std::cout << "CSRF token is empty. Cannot proceed.\n";
        return;
    }
    std::string ticket = getAuthTicket(csrfToken);
    if (ticket.empty()) {
        std::cout << "Auth ticket is empty. Cannot proceed.\n";
        return;
    }

    std::string joinAttemptId = generateUUID();
    if (joinAttemptId.empty()) {
        std::cout << "Failed to generate joinAttemptId.\n";
        return;
    }

    std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(0, 9);
    std::string browserTrackerID;
    for (int i = 0; i < 16; ++i) {
        browserTrackerID += std::to_string(dist(rng));
    }

    std::string encodedUrl = "https://www.roblox.com/Game/PlaceLauncher.ashx?request=RequestFollowUser&browserTrackerId=" + browserTrackerID +
        "&userId=" + userId +
        "&followUserId=" + userId +
        "&joinAttemptId=" + joinAttemptId +
        "&joinAttemptOrigin=JoinUser";

    launchRobloxClient(encodedUrl, ticket, browserTrackerID);
}

void handleRobloxPlayerUrl(const std::string& robloxPlayerUrl) {
    std::cout << "Handling roblox-player URL.\n";
    std::string command = "start \"\" \"" + robloxPlayerUrl + "\"";
    system(command.c_str());
}

void joinPrivateServer(const std::string& vipServerLink) {
    std::string userId = getUserIdFromProfileUrl(vipServerLink);
    if (!userId.empty()) {
        std::cout << "Detected profile URL. User ID: " << userId << "\n";
        followUser(userId);
        return;
    }

    if (vipServerLink.find("roblox-player:") == 0) {
        handleRobloxPlayerUrl(vipServerLink);
        return;
    }

    std::string usernamePattern = "^[a-zA-Z0-9_]{3,20}$";
    std::regex usernameRegex(usernamePattern);
    if (std::regex_match(vipServerLink, usernameRegex)) {
        std::string userIdFromUsername = getUserIdFromUsername(vipServerLink);
        if (!userIdFromUsername.empty()) {
            std::cout << "Detected username. User ID: " << userIdFromUsername << "\n";
            followUser(userIdFromUsername);
            return;
        }
        else {
            std::cout << "Username not found.\n";
            return;
        }
    }

    std::string csrfToken = fetchCsrfToken();
    if (csrfToken.empty()) {
        std::cout << "CSRF token is empty. Cannot proceed.\n";
        return;
    }
    std::string ticket = getAuthTicket(csrfToken);
    if (ticket.empty()) {
        std::cout << "Auth ticket is empty. Cannot proceed.\n";
        return;
    }

    std::string placeId = getPlaceIdFromUrl(vipServerLink);
    std::string privateServerLinkCode = getPrivateServerLinkCode(vipServerLink);
    std::string shareCode, shareType;

    if (privateServerLinkCode.empty()) {
        try {
            std::regex rgx(R"(https?://www\.roblox\.com/share\?code=([\w-]+)&type=(\w+))");
            std::smatch match;
            if (std::regex_search(vipServerLink, match, rgx)) {
                shareCode = match[1];
                shareType = match[2];
            }
            else {
                std::cout << "Not a valid link.\n";
                return;
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error parsing share code and type: " << e.what() << "\n";
            return;
        }
        ShareLinkData resolvedData = resolveShareLinkCode(shareCode, shareType, csrfToken);
        if (resolvedData.privateServerLinkCode.empty() && resolvedData.jobId.empty()) {
            std::cout << "VIP Server link is invalid.\n";
            return;
        }
        privateServerLinkCode = resolvedData.privateServerLinkCode;
        placeId = resolvedData.placeId;
        std::string jobId = resolvedData.jobId;
        std::string referredByUserId = resolvedData.referredByUserId;

        std::string joinAttemptId = generateUUID();
        if (joinAttemptId.empty()) {
            std::cout << "Failed to generate joinAttemptId.\n";
            return;
        }

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 9);
        std::string browserTrackerID;
        for (int i = 0; i < 16; ++i) {
            browserTrackerID += std::to_string(dist(rng));
        }

        std::string encodedUrl;
        if (!privateServerLinkCode.empty()) {
            std::string accessCode = retrieveAccessCode(placeId, privateServerLinkCode, csrfToken);
            if (accessCode.empty()) {
                std::cout << "Access code:\n" << accessCode << "\n";
                std::cout << "Access code retrieval failed.\n";
                return;
            }
            encodedUrl = "https://assetgame.roblox.com/game/PlaceLauncher.ashx?request=RequestPrivateGame&placeId=" + placeId +
                "&accessCode=" + accessCode +
                "&linkCode=" + privateServerLinkCode +
                "&joinAttemptId=" + joinAttemptId;
        }
        else if (!jobId.empty()) {
            encodedUrl = "https://www.roblox.com/Game/PlaceLauncher.ashx?request=RequestGameJob&browserTrackerId=" + browserTrackerID +
                "&placeId=" + placeId +
                "&gameId=" + jobId +
                "&isPlayTogetherGame=true" +
                "&joinAttemptId=" + joinAttemptId +
                "&joinAttemptOrigin=shareLinks";
            if (!referredByUserId.empty()) {
                encodedUrl += "&referredByPlayerId=" + referredByUserId;
            }
        }
        else {
            encodedUrl = "https://assetgame.roblox.com/game/PlaceLauncher.ashx?request=RequestGame&browserTrackerId=" + browserTrackerID +
                "&placeId=" + placeId +
                "&isPlayTogetherGame=false" +
                "&joinAttemptId=" + joinAttemptId;
        }

        launchRobloxClient(encodedUrl, ticket, browserTrackerID);
        return;
    }
    else {
        std::string joinAttemptId = generateUUID();
        if (joinAttemptId.empty()) {
            std::cout << "Failed to generate joinAttemptId.\n";
            return;
        }

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> dist(0, 9);
        std::string browserTrackerID;
        for (int i = 0; i < 16; ++i) {
            browserTrackerID += std::to_string(dist(rng));
        }

        std::string accessCode = retrieveAccessCode(placeId, privateServerLinkCode, csrfToken);
        if (accessCode.empty()) {
            std::cout << "Access code retrieval failed.\n";
            return;
        }
        std::string encodedUrl = "https://assetgame.roblox.com/game/PlaceLauncher.ashx?request=RequestPrivateGame&placeId=" + placeId +
            "&accessCode=" + accessCode +
            "&linkCode=" + privateServerLinkCode +
            "&joinAttemptId=" + joinAttemptId;
        launchRobloxClient(encodedUrl, ticket, browserTrackerID);
    }
}
