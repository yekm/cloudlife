#pragma once

#ifndef _WIN32
#include <unistd.h>
#else
#include <cstring>

extern char* optarg;

int getopt(int argc, char* const argv[], const char* optstring);
#endif
