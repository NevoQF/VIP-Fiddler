#ifndef GLOBALZ_H
#define GLOBALZ_H

#include <chrono>
#include <string>
#include <vector>
#include <windows.h>

extern NOTIFYICONDATA nid;
extern std::chrono::time_point<std::chrono::high_resolution_clock> startTime;
extern std::string KEYBIND;
extern std::string ROBLOSECURITY;
extern std::string csrfTokenCache;
extern long long csrfTokenLastFetched;

#endif
