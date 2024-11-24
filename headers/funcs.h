// Messy? I know. :')
#ifndef UTILS_H
#define UTILS_H

#include <string>
#include <vector>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
std::vector<std::string> fetchWhitelist(const std::string& url);
int getVirtualKeyCode(const std::string& keyName);
std::pair<std::string, std::string> getUserIDAndUsernameFromCookie(const std::string& cookie);
std::string fetchCsrfToken();
std::string generateUUID();
void launchRobloxClient(const std::string& encodedUrl, const std::string& ticket, const std::string& browserTrackerID);
std::string parseAccessCode(const std::string& htmlContent);
std::string getAuthTicket(const std::string& csrfToken);
std::string getClipboardText();
std::string extractBody(const std::string& response);
std::string getPlaceIdFromUrl(const std::string& link);
std::string getPrivateServerLinkCode(const std::string& link);
struct ShareLinkData;
std::string extractFirstJson(const std::string& response);
ShareLinkData resolveShareLinkCode(const std::string& shareCode, const std::string& shareType, const std::string& csrfToken);
std::string retrieveAccessCode(const std::string& placeId, const std::string& linkCode, const std::string& csrfToken);
std::string getUserIdFromProfileUrl(const std::string& link);
std::string getUserIdFromUsername(const std::string& username);
void followUser(const std::string& userId);
void handleRobloxPlayerUrl(const std::string& robloxPlayerUrl);
void joinPrivateServer(const std::string& vipServerLink);
void handleKeybindPress(const std::string& KEYBIND);

#endif
