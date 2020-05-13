/* Forward-declare our functions so users can mention them in their
 * configs at the top of the file rather than near the bottom. */

static char *get_title(char** label),
            *get_bar(char** label),
            *get_os(char** label),
            *get_kernel(char** label),
            *get_host(char** label),
            *get_uptime(char** label),
            *get_battery_1_percentage(char** label),
            *get_battery_2_percentage(char** label),
            *get_packages_pacman(char** label),
            *get_shell(char** label),
            *get_resolution(char** label),
            *get_terminal(char** label),
            *get_cpu(char** label),
            *get_gpu1(char** label),
            *get_gpu2(char** label),
            *get_memory(char** label),
            *get_disk_usage_root(char** label),
            *get_disk_usage_home(char** label),
            *get_colors1(char** label),
            *get_colors2(char** label),
            *spacer(char** label);

#define SPACER {"", spacer, false},
#define REMOVE(A) { (A), NULL, sizeof(A) - 1 , 0 }
#define REPLACE(A, B) { (A), (B), sizeof(A) - 1, sizeof(B) - 1 }
