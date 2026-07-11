#pragma once

#if defined Q_OS_WINRT || defined Q_OS_WIN
#include "platform/win/launcher_win.h"
#else
#include "platform/linux/launcher_linux.h"
#endif 

