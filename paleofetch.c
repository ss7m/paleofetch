#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pci/pci.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "paleofetch.h"
#include "config.h"

#define BUF_SIZE 150

struct conf {
    char *label, *(*function)();
    bool cached;
} config[] = CONFIG;

Display *display;
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
 * Replaces the first newline character with null terminator
 */
void remove_newline(char *s) {
    while (*s != '\0' && *s != '\n')
        s++;
    *s = '\0';
}

/*
 * Cleans up repeated spaces in a string
 */
void truncate_spaces(char *str) {
    int src = 0, dst = 0;

    while(*(str + dst) != '\0') {
        *(str + src) = *(str + dst);
        if(*(str + (dst++)) == ' ')
            while(*(str + dst) == ' ') dst++;

        src++;
    }

    *(str +src) = '\0';
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
    snprintf(title, BUF_SIZE, COLOR"%s\e[0m@"COLOR"%s", username, hostname);

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

    FILE *product_version = fopen("/sys/devices/virtual/dmi/id/product_version", "r");

    if(product_version == NULL) {
        status = -1;
        halt_and_catch_fire("unable to open product version file");
    }

    char version[BUF_SIZE];

    fread(version, 1, BUF_SIZE, product_version);
    fclose(product_version);

    remove_newline(host);
    remove_newline(version);

    strcat(host, " ");
    strcat(host, version);

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
    int screen = DefaultScreen(display);

    int width = DisplayWidth(display, screen);
    int height = DisplayHeight(display, screen);

    char *resolution = malloc(BUF_SIZE);
    snprintf(resolution, BUF_SIZE, "%dx%d", width, height);

    return resolution;
}

char *get_terminal() {
    unsigned char *prop;
    unsigned long _, // not unused, but we don't need the results
                  window = RootWindow(display, XDefaultScreen(display));
    Atom a,
         active = XInternAtom(display, "_NET_ACTIVE_WINDOW", True),
         class = XInternAtom(display, "WM_CLASS", True);

#define GetProp(property) \
    XGetWindowProperty(display, window, property, 0, 64, 0, 0, &a, (int *)&_, &_, &_, &prop);

    GetProp(active);
    window = (prop[3] << 24) + (prop[2] << 16) + (prop[1] << 8) + prop[0];
    free(prop);
    GetProp(class);

#undef GetProp

    char *terminal = malloc(BUF_SIZE);
    snprintf(terminal, BUF_SIZE, "%s", prop);
    free(prop);

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
    truncate_spaces(cpu);

    return cpu;
}

char *get_gpu() {
    // inspired by https://github.com/pciutils/pciutils/edit/master/example.c
    /* it seems that pci_lookup_name needs to be given a buffer, but I can't for the life of my figure out what its for */
    char buffer[BUF_SIZE], *gpu = malloc(BUF_SIZE);
    struct pci_access *pacc;
    struct pci_dev *dev;

    pacc = pci_alloc();
    pci_init(pacc);
    pci_scan_bus(pacc);
    dev = pacc->devices;

    while(dev != NULL) {
        pci_fill_info(dev, PCI_FILL_IDENT);
        if(strcmp("VGA compatible controller", pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_CLASS, dev->device_class)) == 0) {
            strncpy(gpu, pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id), BUF_SIZE);
            break;
        }

        dev = dev->next;
    }

    remove_substring(gpu, "Corporation", 11);
    truncate_spaces(gpu);

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

char *spacer() {
    return calloc(1, 1); // freeable, null-terminated string of length 1
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

/* This isn't especially robust, but as long as we're the only one writing
 * to our cache file, the format is simple, effective, and fast. One way
 * we might get in trouble would be if the user decided not to have any
 * sort of sigil (like ':') after their labels. */
char *search_cache(char *cache_data, char *label) {
    char *start = strstr(cache_data, label) + strlen(label);
    char *end = strchr(start, ';');

    char *buf = calloc(1, BUF_SIZE);
    // skip past the '=' and stop just before the ';'
    strncpy(buf, start + 1, end - start - 1);

    return buf;
}

char *get_value(struct conf c, int read_cache, char *cache_data) {
    char *value;

    // If the user's config specifies that this value should be cached
    if(c.cached && read_cache) // and we have a cache to read from
        value = search_cache(cache_data, c.label); // grab it from the cache
    else {
        // Otherwise, call the associated function to get the value
        value = c.function();
        if(c.cached) { // and append it to our cache data if appropriate
            char *buf = malloc(BUF_SIZE);
            sprintf(buf, "%s=%s;", c.label, value);
            strcat(cache_data, buf);
            free(buf);
        }
    }

    return value;
}

int main(int argc, char *argv[]) {
    char *cache, *cache_data = NULL;
    FILE *cache_file;
    int read_cache;

    status = uname(&uname_info);
    halt_and_catch_fire("uname failed");
    status = sysinfo(&my_sysinfo);
    halt_and_catch_fire("sysinfo failed");
    display = XOpenDisplay(NULL);
    if(display == NULL) {
        status = -1;
        halt_and_catch_fire("XOpenDisplay failed");
    }

    cache = get_cache_file();
    if(argc == 2 && strcmp(argv[1], "--recache") == 0)
        read_cache = 0;
    else {
        cache_file = fopen(cache, "r");
        read_cache = cache_file != NULL;
    }

    if(!read_cache)
        cache_data = calloc(4, BUF_SIZE); // should be enough
    else {
        size_t len; /* unused */
        getline(&cache_data, &len, cache_file);
        fclose(cache_file); // We just need the first (and only) line.
    }

#define COUNT(x) (int)(sizeof x / sizeof *x)

    for (int i = 0; i < COUNT(LOGO); i++) {
        // If we've run out of information to show...
        if(i >= COUNT(config)) // just print the next line of the logo
            puts(LOGO[i]);
        else {
            // Otherwise, we've got a bit of work to do.
            char *label = config[i].label,
                 *value = get_value(config[i], read_cache, cache_data);
            printf(COLOR"%s%s\e[0m%s\n", LOGO[i], label, value);
            free(value);

        }
    }
    puts("\e[0m");

    /* Write out our cache data (if we have any). */
    if(!read_cache && *cache_data) {
        cache_file = fopen(cache, "w");
        fprintf(cache_file, "%s", cache_data);
        fclose(cache_file);
    }

    free(cache);
    free(cache_data);
    XCloseDisplay(display);

    return 0;
}
