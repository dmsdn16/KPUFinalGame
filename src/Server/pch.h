// pch.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#ifndef _PCH_H_
#define _PCH_H_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <WinSock2.h>
// C++ 런타임 헤더 파일입니다.
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <thread>

#endif // !_PCH_H_