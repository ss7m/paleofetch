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
    { "Memory: ",     get_memory,     false }, \
    SPACER \
    { "",             get_colors1,    false }, \
    { "",             get_colors2,    false }, \
};

#define CPU_REMOVE \
{ \
   /* string   length */ \
   { "(R)",         3 }, \
   { "(TM)",        4 }, \
   { "Dual-Core",   9 }, \
   { "Quad-Core",   9 }, \
   { "Six-Core",    8 }, \
   { "Eight-Core", 10 }, \
   { "Core",        4 }, \
   { "CPU",         3 }, \
};

#define GPU_REMOVE \
{ \
   /* string    length */ \
   { "Corporation", 11 }, \
};
