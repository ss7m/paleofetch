#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pci/pci.h>

#include <X11/Xlib.h>

#define DISTRO "Arch"
#define BUF_SIZE 150

// I copy pasted this from neofetch, in case you were curious
#define FORMAT_STR \
"\e[1;36m                  -`                    \e[0m%s\n"\
"\e[1;36m                 .o+`                   \e[0m%s\n"\
"\e[1;36m                `ooo/                   OS: \e[0m%s\n"\
"\e[1;36m               `+oooo:                  Host: \e[0m%s\n"\
"\e[1;36m              `+oooooo:                 Kernel: \e[0m%s\n"\
"\e[1;36m              -+oooooo+:                Uptime: \e[0m%s\n"\
"\e[1;36m            `/:-:++oooo+:\n"\
"\e[1;36m           `/++++/+++++++:              Packages: \e[0m%s\n"\
"\e[1;36m          `/++++++++++++++:             Shell: \e[0m%s\n"\
"\e[1;36m         `/+++ooooooooooooo/`           Resolution: \e[0m%s\n"\
"\e[1;36m        ./ooosssso++osssssso+`          Terminal: \e[0m%s\n"\
"\e[1;36m       .oossssso-````/ossssss+`\n"\
"\e[1;36m      -osssssso.      :ssssssso.        CPU: \e[0m%s\n"\
"\e[1;36m     :osssssss/        osssso+++.       GPU: \e[0m%s\n"\
"\e[1;36m    /ossssssss/        +ssssooo/-       Memory: \e[0m%s\n"\
"\e[1;36m  `/ossssso+/:-        -:/+osssso+-\n"\
"\e[1;36m `+sso+:-`                 `.-/+oso:    %s\n"\
"\e[1;36m`++:.                           `-/+/   %s\n"\
"\e[1;36m.`                                 `/\e[0m\n\n"

// TODO: Finish it

struct utsname uname_info;
struct sysinfo my_sysinfo;
int title_length;
int status;

void halt_and_catch_fire(const char *message) {
    if(status != 0) {
        printf("%s\n", message);
        exit(status);
    }
}

/*
 * Returns index of substring in str, if it is there
 * Assumes that strlen(substring) >= len
 */
int contains_substring(const char *str, const char*substring, size_t len) {
    if(len == 0) return -1;

    /* search for substring */
    int offset = 0;
    for(;;) {
        int match = 1;
        for(size_t i = 0; i < len; i++) {
            if(*(str+offset+i) == '\0') return -1;
            else if(*(str+offset+i) != *(substring+i)) {
                match = 0;
                break;
            }
        }

        if(match) break;
        offset++;
    }

    return offset;
}

/*
 * Removes the first len characters of substring from str
 * Assumes that strlen(substring) >= len
 * Returns index where substring was found, or -1 if substring isn't found
 */
void remove_substring(char *str, const char* substring, size_t len) {
    /* shift over the rest of the string to remove substring */
    int offset = contains_substring(str, substring, len);
    if(offset < 0) return;

    int i = 0;
    for(;;) {
        if(*(str+offset+i) == '\0') break;
        *(str+offset+i) = *(str+offset+i+len);
        i++;
    }
}

char *get_title() {
    // reduce the maximum size for these, so that we don't over-fill the title string
    char hostname[BUF_SIZE / 3];
    status = gethostname(hostname, BUF_SIZE / 3);
    halt_and_catch_fire("unable to retrieve host name");

    char username[BUF_SIZE / 3];
    status = getlogin_r(username, BUF_SIZE / 3);
    halt_and_catch_fire("unable to retrieve login name");

    title_length = strlen(hostname) + strlen(username) + 1;

    char *title = malloc(BUF_SIZE);
    snprintf(title, BUF_SIZE, "\e[1;36m%s\e[0m@\e[1;36m%s", username, hostname);

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

    if(product_name == NULL) {
        status = -1;
        halt_and_catch_fire("unable to open product name file");
    }

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

    if(dirp == NULL) {
        status = -1;
        halt_and_catch_fire("Do you not have pacman installed? How did you find this?\n"
                "Please email samfbarr@outlook.com with the details of how you got here.\n"
                "This information will be very useful for my upcoming demographics survey.");
    }

    while((entry = readdir(dirp)) != NULL) {
        if(entry->d_type == DT_DIR) num_packages++;
    }
    num_packages -= 2; // accounting for . and ..

    status = closedir(dirp);

    char *packages = malloc(BUF_SIZE);
    snprintf(packages, BUF_SIZE, "%d (pacman)", num_packages);

    return packages;
}

char *get_shell() {
    char *shell = malloc(BUF_SIZE);
    strncpy(shell, strrchr(getenv("SHELL"), '/') + 1, BUF_SIZE);
    return shell;
}

char *get_resolution() {
    Display *display = XOpenDisplay(NULL);
    if(display == NULL) {
        status = -1;
        halt_and_catch_fire("XOpenDisplay failed");
    }
    int screen = DefaultScreen(display);

    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);
    XCloseDisplay(display);

    char *resolution = malloc(BUF_SIZE);
    snprintf(resolution, BUF_SIZE, "%dx%d", width, height);

    return resolution;
}

char *get_terminal() {
    char *terminal = malloc(BUF_SIZE);
    strncpy(terminal, getenv("TERM"), BUF_SIZE);
    return terminal;
}

char *get_cpu() {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "r"); /* read from cpu info */
    if(cpuinfo == NULL) {
        status = -1;
        halt_and_catch_fire("Unable to open cpuinfo");
    }

    char *cpu_model = malloc(BUF_SIZE / 2);
    char *line = NULL;
    size_t len; /* unused */
    int num_cores = 0;

    /* read the model name into cpu_model, and increment num_cores every time model name is found */
    while(getline(&line, &len, cpuinfo) != -1) {
        num_cores += sscanf(line, "model name	: %[^@] @", cpu_model);
    }
    free(line);
    fclose(cpuinfo);

    char *cpu = malloc(BUF_SIZE);
    snprintf(cpu, BUF_SIZE, "%s(%d)", cpu_model, num_cores);
    free(cpu_model);

    /* remove unneeded information */
    remove_substring(cpu, "(R)", 3);
    remove_substring(cpu, "(TM)", 4);
    remove_substring(cpu, "Core", 4);
    remove_substring(cpu, "CPU", 3);

    return cpu;
}

char *get_gpu() {
    // inspired by https://github.com/pciutils/pciutils/edit/master/example.c
    char *gpu = malloc(BUF_SIZE);
    struct pci_access *pacc;
    struct pci_dev *dev;

    pacc = pci_alloc();
    pci_init(pacc);
    pci_scan_bus(pacc);
    dev = pacc->devices;

    while(dev != NULL) {
        pci_fill_info(dev, PCI_FILL_IDENT);
        pci_lookup_name(pacc, gpu, BUF_SIZE, PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id);
        /* as far as I know, this is the only way to check if its a graphics card */
        if(contains_substring(gpu, "Graphics", 8) >= 0) break;

        dev = dev->next;
    }

    remove_substring(gpu, "Corporation ", 12);

    pci_cleanup(pacc);
    return gpu;
}

char *get_memory() {
    int total_memory, used_memory;
    int total, shared, memfree, buffers, cached, reclaimable;

    FILE *meminfo = fopen("/proc/meminfo", "r"); /* get infomation from meminfo */
    if(meminfo == NULL) {
        status = -1;
        halt_and_catch_fire("Unable to open meminfo");
    }

    /* We parse through all lines of meminfo and scan for the information we need */
    char *line = NULL; // allocation handled automatically by getline()
    size_t len; /* unused */

    /* parse until EOF */
    while (getline(&line, &len, meminfo) != -1) {
        /* if sscanf doesn't find a match, pointer is untouched */
        sscanf(line, "MemTotal: %d", &total);
        sscanf(line, "Shmem: %d", &shared);
        sscanf(line, "MemFree: %d", &memfree);
        sscanf(line, "Buffers: %d", &buffers);
        sscanf(line, "Cached: %d", &cached);
        sscanf(line, "SReclaimable: %d", &reclaimable);
    }

    free(line);

    fclose(meminfo);

    /* use same calculation as neofetch */
    used_memory = (total + shared - memfree - buffers - cached - reclaimable) / 1024;
    total_memory = total / 1024;
    int percentage = (int) (100 * (used_memory / (double) total_memory));

    char *memory = malloc(BUF_SIZE);
    snprintf(memory, BUF_SIZE, "%dMiB / %dMiB (%d%%)", used_memory, total_memory, percentage);

    return memory;
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
    char *colors2 = malloc(BUF_SIZE);
    char *s = colors2;

    for(int i = 8; i < 16; i++) {
        sprintf(s, "\e[48;5;%dm   ", i);
        s += 12 + (i >= 10 ? 1 : 0);
    }
    snprintf(s, 5, "\e[0m");

    return colors2;
}

char *get_cache_file() {
    char *cache_file = malloc(BUF_SIZE);
    char *env = getenv("XDG_CACHE_HOME");
    if(env == NULL)
        snprintf(cache_file, BUF_SIZE, "%s/.cache/paleofetch", getenv("HOME"));
    else
        snprintf(cache_file, BUF_SIZE, "%s/paleofetch", env);

    return cache_file;
}

int main(int argc, char *argv[]) {
    char *cache;
    FILE *cache_file;
    char *title, *bar, *os, *kernel, *host, *uptime, *packages, *shell, *resolution, *terminal, *cpu, *gpu, *memory, *colors1, *colors2;
    int read_cache;

    status = uname(&uname_info);
    halt_and_catch_fire("uname failed");
    status = sysinfo(&my_sysinfo);
    halt_and_catch_fire("sysinfo failed");

    cache = get_cache_file();
    if(argc == 2 && strcmp(argv[1], "--recache") == 0)
        read_cache = 0;
    else {
        cache_file = fopen(cache, "r");
        read_cache = cache_file != NULL;
    }

    if(read_cache) {
        os = malloc(BUF_SIZE);
        host = malloc(BUF_SIZE);
        kernel = malloc(BUF_SIZE);
        cpu = malloc(BUF_SIZE);
        gpu = malloc(BUF_SIZE);
        fscanf(cache_file, "OS: %[^\n] HOST: %[^\n] KERNEL: %[^\n] CPU: %[^\n] GPU: %[^\n]",
                os, host, kernel, cpu, gpu);
        fclose(cache_file);
    }
    else {
        os = get_os();
        host = get_host();
        kernel = get_kernel();
        cpu = get_cpu();
        gpu = get_gpu();
    }

    title = get_title();
    bar = get_bar();
    uptime = get_uptime();
    packages = get_packages();
    shell = get_shell();
    resolution = get_resolution();
    terminal = get_terminal();
    memory = get_memory();
    colors1 = get_colors1();
    colors2 = get_colors2();

    printf(FORMAT_STR, title, bar, os, host, kernel, uptime, packages, shell, resolution, terminal, cpu, gpu, memory, colors1, colors2);

    if(!read_cache) {
        cache_file = fopen(cache, "w");
        fprintf(cache_file, "OS: %s\nHOST: %s\nKERNEL: %s\nCPU: %s\nGPU: %s\n",
                os, host, kernel, cpu, gpu);
        fclose(cache_file);
    }

    free(title);
    free(bar);
    free(os);
    free(kernel);
    free(host);
    free(uptime);
    free(packages);
    free(shell);
    free(resolution);
    free(terminal);
    free(cpu);
    free(gpu);
    free(memory);
    free(colors1);
    free(colors2);

    free(cache);
}
