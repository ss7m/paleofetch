#include "logos/arch.h"
#define COLOR "\e[1;36m"

#define CONFIG \
{ \
   /* name            function        cached */\
    { "",             get_title,      false }, \
    { "",             get_bar,        false }, \
    { "OS: ",         get_os,         true  }, \
    { "Host: ",       get_host,       true  }, \
    { "Kernel: ",     get_kernel,     true  }, \
    { "Uptime: ",     get_uptime,     false }, \
    SPACER \
    { "Packages: ",   get_packages,   false }, \
    { "Shell: ",      get_shell,      false }, \
    { "Resolution: ", get_resolution, false }, \
    { "Terminal: ",   get_terminal,   false }, \
    SPACER \
    { "CPU: ",        get_cpu,        true  }, \
    { "GPU: ",        get_gpu1,       true  }, \
    { "GPU: ",        get_gpu2,       true  }, \
    { "Memory: ",     get_memory,     false }, \
    SPACER \
    { "",             get_colors1,    false }, \
    { "",             get_colors2,    false }, \
};
