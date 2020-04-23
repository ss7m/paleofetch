/* Forward-declare our functions so users can mention them in their
 * configs at the top of the file rather than near the bottom. */

char *get_title(),
     *get_bar(),
     *get_os(),
     *get_kernel(),
     *get_host(),
     *get_uptime(),
     *get_packages(),
     *get_shell(),
     *get_resolution(),
     *get_terminal(),
     *get_cpu(),
     *get_gpu(),
     *get_memory(),
     *get_colors1(),
     *get_colors2(),
     *spacer();

#define SPACER {"", spacer, false},
