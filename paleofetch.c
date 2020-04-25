#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h>
#include <pci/pci.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "paleofetch.h"
#include "config.h"

#define BUF_SIZE 150
#define COUNT(x) (int)(sizeof x / sizeof *x)

#define halt_and_catch_fire(fmt, ...) \
    do { \
        if(status != 0) { \
            fprintf(stderr, "paleofetch: " fmt "\n", ##__VA_ARGS__); \
            exit(status); \
        } \
    } while(0)

struct conf {
    char *label, *(*function)();
    bool cached;
} config[] = CONFIG;

typedef struct {
    char *substring;
    size_t length;
} STRING_REMOVE;

STRING_REMOVE cpu_remove[] = CPU_REMOVE;
STRING_REMOVE gpu_remove[] = GPU_REMOVE;

Display *display;
struct utsname uname_info;
struct sysinfo my_sysinfo;
int title_length;
int status;

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
 * Removes the first len characters of substring from str
 * Assumes that strlen(substring) >= len
 * Returns index where substring was found, or -1 if substring isn't found
 */
void remove_substring(char *str, const char* substring, size_t len) {
    /* shift over the rest of the string to remove substring */
    char *sub = strstr(str, substring);
    if(sub == NULL) return;

    int i = 0;
    do *(sub+i) = *(sub+i+len);
    while(*(sub+(++i)) != '\0');
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
    char *os = malloc(BUF_SIZE),
         *name = malloc(BUF_SIZE),
         *line = NULL;
    size_t len;
    FILE *os_release = fopen("/etc/os-release", "r");
    if(os_release == NULL) {
        status = -1;
        halt_and_catch_fire("unable to open /etc/os-release");
    }

    while (getline(&line, &len, os_release) != -1) {
        if (sscanf(line, "NAME=\"%[^\"]+", name) > 0) break;
    }

    free(line);
    fclose(os_release);
    snprintf(os, BUF_SIZE, "%s %s", name, uname_info.machine);
    free(name);

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
    struct { char *name; int secs; } units[] = {
        { "day",  60 * 60 * 24 },
        { "hour", 60 * 60 },
        { "min",  60 },
    };

    int n, len = 0;
    char *uptime = malloc(BUF_SIZE);
    for (int i = 0; i < 3; ++i ) {
        if ((n = seconds / units[i].secs) || i == 2) /* always print minutes */
            len += snprintf(uptime + len, BUF_SIZE - len, 
                            "%d %s%s, ", n, units[i].name, n != 1 ? "s": "");
        seconds %= units[i].secs;
    }

    // null-terminate at the trailing comma
    uptime[len - 2] = '\0';
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
    char *shell_path = getenv("SHELL");
    char *shell_name = strrchr(getenv("SHELL"), '/');

    if(shell_name == NULL) /* if $SHELL doesn't have a '/' */
        strncpy(shell, shell_path, BUF_SIZE); /* copy the whole thing over */
    else
        strncpy(shell, shell_name + 1, BUF_SIZE); /* o/w copy past the last '/' */

    return shell;
}

char *get_resolution() {
    int screen, width, height;
    char *resolution = malloc(BUF_SIZE);
    
    if (display != NULL) {
        screen = DefaultScreen(display);
    
        width = DisplayWidth(display, screen);
        height = DisplayHeight(display, screen);

        snprintf(resolution, BUF_SIZE, "%dx%d", width, height);
    } else {
        DIR *dir;
        struct dirent *entry;
        char dir_name[] = "/sys/class/drm";
        char modes_file_name[BUF_SIZE * 2];
        FILE *modes;
        char *line = NULL;
        size_t len;
        
        /* preload resolution with empty string, in case we cant find a resolution through parsing */
        strncpy(resolution, "", BUF_SIZE);

        dir = opendir(dir_name);
        if (dir == NULL) {
            status = -1;
            halt_and_catch_fire("Could not open /sys/class/drm to determine resolution in tty mode.");
        }
        /* parse through all directories and look for a non empty modes file */
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_LNK) {
                snprintf(modes_file_name, BUF_SIZE * 2, "%s/%s/modes", dir_name, entry->d_name);

                modes = fopen(modes_file_name, "r");
                if (modes != NULL) {
                    if (getline(&line, &len, modes) != -1) {
                        strncpy(resolution, line, BUF_SIZE);
                        remove_newline(resolution);

                        free(line);
                        fclose(modes);

                        break;
                    }

                    fclose(modes);
                }
            }
        }
        
        closedir(dir);
    }

    return resolution;
}

char *get_terminal() {
    unsigned char *prop;
    char *terminal = malloc(BUF_SIZE);

    /* check if xserver is running or if we are running in a straight tty */
    if (display != NULL) {   

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
        if(!window) goto terminal_fallback;
        GetProp(class);

#undef GetProp

        snprintf(terminal, BUF_SIZE, "%s", prop);
        free(prop);
    } else {
terminal_fallback:
        strncpy(terminal, getenv("TERM"), BUF_SIZE); /* fallback to old method */
    }

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
    int num_cores = 0, cpu_freq, prec = 3;
    double freq;

    /* read the model name into cpu_model, and increment num_cores every time model name is found */
    while(getline(&line, &len, cpuinfo) != -1) {
        num_cores += sscanf(line, "model name	: %[^\n@]", cpu_model);
    }
    free(line);
    fclose(cpuinfo);

    FILE *cpufreq = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");

    if (cpufreq == NULL) {
        status = -1;
        halt_and_catch_fire("Unable to open cpufreq");
    }

    line = NULL;

    if (getline(&line, &len, cpufreq) != -1) {
        sscanf(line, "%d", &cpu_freq);
        cpu_freq /= 1000; // convert kHz to MHz
        freq = cpu_freq / 1000.0; // convert MHz to GHz and cast to double
        while (cpu_freq % 10 == 0) {
            --prec;
            cpu_freq /= 10;
        }
        if (prec == 0) prec = 1; // we don't want zero decimal places 
    } else {
        freq = 0.0; // cpuinfo_max_freq not available?
    }

    free(line);
    fclose(cpufreq);

    /* remove unneeded information */
    for (int i = 0; i < COUNT(cpu_remove); ++i) {
        remove_substring(cpu_model, cpu_remove[i].substring, cpu_remove[i].length);
    }

    char *cpu = malloc(BUF_SIZE);
    snprintf(cpu, BUF_SIZE, "%s (%d) @ %.*fGHz", cpu_model, num_cores, prec, freq);
    free(cpu_model);

    truncate_spaces(cpu);
    return cpu;
}

char *find_gpu(int index) {
    // inspired by https://github.com/pciutils/pciutils/edit/master/example.c
    /* it seems that pci_lookup_name needs to be given a buffer, but I can't for the life of my figure out what its for */
    char buffer[BUF_SIZE], *device_class, *gpu = malloc(BUF_SIZE);
    struct pci_access *pacc;
    struct pci_dev *dev;
    int gpu_index = 0;
    bool found = false;

    pacc = pci_alloc();
    pci_init(pacc);
    pci_scan_bus(pacc);
    dev = pacc->devices;

    while(dev != NULL) {
        pci_fill_info(dev, PCI_FILL_IDENT);
        device_class = pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_CLASS, dev->device_class);
        if(strcmp("VGA compatible controller", device_class) == 0 || strcmp("3D controller", device_class) == 0) {
            strncpy(gpu, pci_lookup_name(pacc, buffer, sizeof(buffer), PCI_LOOKUP_DEVICE | PCI_LOOKUP_VENDOR, dev->vendor_id, dev->device_id), BUF_SIZE);
            if(gpu_index == index) {
                found = true;
                break;
            } else {
                gpu_index++;
            }
        }

        dev = dev->next;
    }

    if (found == false) *gpu = '\0'; // empty string, so it will not be printed

    pci_cleanup(pacc);

    /* remove unneeded information */
    for (int i = 0; i < COUNT(gpu_remove); ++i) {
        remove_substring(gpu, gpu_remove[i].substring, gpu_remove[i].length);
    }

    truncate_spaces(gpu);

    return gpu;
}

char *get_gpu1() {
    return find_gpu(0);
}

char *get_gpu2() {
    return find_gpu(1);
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
    char *start = strstr(cache_data, label);
    if(start == NULL) {
        status = ENODATA;
        halt_and_catch_fire("cache miss on key '%s'; need to --recache?", label);
    }
    start += strlen(label);
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

    int offset = 0;

    for (int i = 0; i < COUNT(LOGO); i++) {
        // If we've run out of information to show...
        if(i >= COUNT(config) - offset) // just print the next line of the logo
            printf(COLOR"%s\n", LOGO[i]);
        else {
            // Otherwise, we've got a bit of work to do.
            char *label = config[i+offset].label,
                 *value = get_value(config[i+offset], read_cache, cache_data);
            if (strcmp(value, "") != 0) { // check if value is an empty string
                printf(COLOR"%s%s\e[0m%s\n", LOGO[i], label, value); // just print if not empty
            } else {
                if (strcmp(label, "") != 0) { // check if label is empty, otherwise it's a spacer
                    ++offset; // print next line of information
                    free(value); // free memory allocated for empty value
                    label = config[i+offset].label; // read new label and value
                    value = get_value(config[i+offset], read_cache, cache_data);
                }
                printf(COLOR"%s%s\e[0m%s\n", LOGO[i], label, value);
            }
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
    if(display != NULL) { 
        XCloseDisplay(display);
    }

    return 0;
}
