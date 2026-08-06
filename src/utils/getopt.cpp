#include "getopt.hpp"

#ifdef _WIN32

static int optarg_idx = 1;
char* optarg = nullptr;

int getopt(int argc, char* const argv[], const char* optstring)
{
    if (optarg_idx >= argc || argv[optarg_idx][0] != '-' || argv[optarg_idx][1] == '\0') {
        return -1;
    }

    char opt = argv[optarg_idx][1];
    const char* p = strchr(optstring, opt);

    if (p == nullptr) {
        optarg_idx++;
        return '?';
    }

    if (p[1] == ':') {
        if (argv[optarg_idx][2] != '\0') {
            optarg = &argv[optarg_idx][2];
            optarg_idx++;
        } else if (optarg_idx + 1 < argc) {
            optarg = argv[optarg_idx + 1];
            optarg_idx += 2;
        } else {
            optarg_idx++;
            return '?';
        }
    } else {
        optarg = nullptr;
        optarg_idx++;
    }

    return opt;
}

#endif
