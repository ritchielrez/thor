#ifndef DEFINES_H_INCLUDED
#define DEFINES_H_INCLUDED

#define BIN_NAME_MAX_SZ 16

#define nullptr (void *)0

#define INTERNAL_DEF static

#if defined(_WIN32) || defined(__CYGWIN__)
// This directive tells the Windows headers to exclude some of the less
// frequently used APIs from the Windows headers
#define WIN32_LEAN_AND_MEAN
#define BUILD_WINDOWS
#include <windows.h>
#elif defined(__linux__)
#define BUILD_LINUX
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#else
#error Unknown operating system, only Windows and Linux is currently supported
#endif

#define MAX(a, b) a > b ? a : b
#define MIN(a, b) a < b ? a : b

#endif  // DEFINES_H_INCLUDED
