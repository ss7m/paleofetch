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
     *get_gpu1(),
     *get_gpu2(),
     *get_memory(),
     *get_colors1(),
     *get_colors2(),
     *spacer();

#define SPACER {"", spacer, false},
#define REMOVE(A) { (A), NULL, sizeof(A) - 1 , 0 }
#define REPLACE(A, B) { (A), (B), sizeof(A) - 1, sizeof(B) - 1 }
