#pragma once

#ifndef _DEBUG
#define LOG(FMT, ...)
#else
#include <cstdio>
#define LOG(FMT, ...) printf(FMT, __VA_ARGS__)
#endif
