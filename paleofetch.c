#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h> // man 2 sysinfo

#include <dirent.h>

#define DISTRO "Arch"
#define BUF_SIZE 150

#define FORMAT_STR \
"\x1b[1;36m                  -`                    \x1b[0m%s\n"\
"\x1b[1;36m                 .o+`                   \x1b[0m%s\n"\
"\x1b[1;36m                `ooo/                   OS: \x1b[0m%s\n"\
"\x1b[1;36m               `+oooo:                  Host: \x1b[0m%s\n"\
"\x1b[1;36m              `+oooooo:                 Kernel: \x1b[0m%s\n"\
"\x1b[1;36m              -+oooooo+:                Uptime: \x1b[0m%s\n"\
"\x1b[1;36m            `/:-:++oooo+:\n"\
"\x1b[1;36m           `/++++/+++++++:              Packages: \x1b[0m%s\n"\
"\x1b[1;36m          `/++++++++++++++:             Shell: \x1b[0m%s\n"\
"\x1b[1;36m         `/+++ooooooooooooo/`           Resolution: \x1b[0m%s\n"\
"\x1b[1;36m        ./ooosssso++osssssso+`          Terminal: \x1b[0m%s\n"\
"\x1b[1;36m       .oossssso-````/ossssss+`\n"\
"\x1b[1;36m      -osssssso.      :ssssssso.        CPU: \x1b[0m%s\n"\
"\x1b[1;36m     :osssssss/        osssso+++.       GPU: \x1b[0m%s\n"\
"\x1b[1;36m    /ossssssss/        +ssssooo/-       Memory: \x1b[0m%s\n"\
"\x1b[1;36m  `/ossssso+/:-        -:/+osssso+-\n"\
"\x1b[1;36m `+sso+:-`                 `.-/+oso:    %s\n"\
"\x1b[1;36m`++:.                           `-/+/   %s\n"\
"\x1b[1;36m.`                                 `/\x1b[0m\n\n"

// TODO: error checking
// TODO: Finish it

struct utsname uname_info;
struct sysinfo my_sysinfo;
int title_length;

char *get_title() {
    // reduce the maximum size for these, so that we don't over-fill the title string
    char hostname[BUF_SIZE / 3];
    gethostname(hostname, BUF_SIZE / 3);
    char username[BUF_SIZE / 3];
    getlogin_r(username, BUF_SIZE / 3);

    title_length = strlen(hostname) + strlen(username) + 1;

    char *title = malloc(BUF_SIZE);
    snprintf(title, BUF_SIZE, "\e[1;36m%s\e[0m@\e[1;36m%s", hostname, username);

    return title;
}

char *get_bar() {
    char *bar = malloc(BUF_SIZE);
    char *s = bar;
    for(int i = 0; i < title_length; i++) *(s++) = '-';
    *s = '\0';
    return bar;
}

char *get_os() {
    char *os = malloc(BUF_SIZE);
    snprintf(os, BUF_SIZE, "%s %s %s", DISTRO, uname_info.sysname, uname_info.machine);
    return os;
}

char *get_kernel() {
    char *kernel = malloc(BUF_SIZE);
    strncpy(kernel, uname_info.release, BUF_SIZE);
    return kernel;
}

char *get_host() {
    FILE *product_name = fopen("/sys/devices/virtual/dmi/id/product_name", "r");
    char *host = malloc(BUF_SIZE);
    fread(host, 1, BUF_SIZE, product_name);
    fclose(product_name);

    // trim trailing newline
    char *s = host;
    while(*(++s) != '\n') ;
    *s = '\0';

    return host;
}

char *get_uptime() {
    long seconds = my_sysinfo.uptime;
    long hours = seconds / 3600;
    long minutes = (seconds / 60) % 60;
    seconds = seconds % 60;

    char *uptime = malloc(BUF_SIZE);

    if(hours > 0)
        snprintf(uptime, BUF_SIZE, "%ld hours, %ld mins", hours, minutes);
    else if(minutes > 0)
        snprintf(uptime, BUF_SIZE, "%ld mins", minutes);
    else
        snprintf(uptime, BUF_SIZE, "%ld secs", seconds);

    return uptime;
}

// full disclosure: I don't know if this is a good idea
char *get_packages() {
    int num_packages = 0;
    DIR * dirp;
    struct dirent *entry;

    dirp = opendir("/var/lib/pacman/local");
    while((entry = readdir(dirp)) != NULL) {
        if(entry->d_type == DT_DIR) num_packages++;
    }
    num_packages -= 2; // accounting for . and ..

    closedir(dirp);

    char *packages = malloc(BUF_SIZE);
    snprintf(packages, BUF_SIZE, "%d (pacman)", num_packages);

    return packages;
}

char *get_shell() {
    char *shell = malloc(BUF_SIZE);
    sscanf(getenv("SHELL"), "/bin/%s", shell);
    return shell;
}

char *get_colors1() {
    char *colors1 = malloc(BUF_SIZE);
    char *s = colors1;

    for(int i = 0; i < 8; i++) {
        sprintf(s, "\e[4%dm   ", i);
        s += 8;
    }
    snprintf(s, 5, "\e[0m");

    return colors1;
}

char *get_colors2() {
    char *colors2 = malloc(2 * BUF_SIZE); // this ends up being a long string
    char *s = colors2;

    for(int i = 8; i < 16; i++) {
        sprintf(s, "\e[48;5;%dm   ", i);
        s += 12 + (i >= 10 ? 1 : 0);
    }
    snprintf(s, 5, "\e[0m");

    return colors2;
}

int main() {
    uname(&uname_info);
    sysinfo(&my_sysinfo);

    char *title = get_title();
    char *bar = get_bar();
    char *os = get_os();
    char *kernel = get_kernel();
    char *host = get_host();
    char *uptime = get_uptime();
    char *packages = get_packages();
    char *shell = get_shell();
    char *colors1 = get_colors1();
    char *colors2 = get_colors2();

    printf(FORMAT_STR, title, bar, os, host, kernel, uptime, packages, shell, "RESOLUTION", "TERMINAL", "CPU", "GPU", "MEMORY", colors1, colors2);

    free(title);
    free(bar);
    free(os);
    free(kernel);
    free(host);
    free(uptime);
    free(packages);
    free(shell);
    free(colors1);
    free(colors2);
}
