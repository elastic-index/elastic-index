#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Return memory usage in bytes */
size_t getMemUsage(void)
{
    size_t size = 0;
    char* line;
    size_t len;
    FILE *f;

    f = fopen("/proc/self/status", "r");
    if (!f) return 1;

    len = 128;
    line = (char*) malloc(len);
 	
    /* Read memory size data from /proc/pid/status */
    while (1) {
        if (getline(&line, &len, f) == -1)
            exit(1);
		
	/* Find VmSize */
        if (!strncmp(line, "VmSize:", 7)) {
           char* vmsize = &line[7];
           len = strlen(vmsize);
           /* Get rid of " kB\n" */
           vmsize[len - 4] = 0;
           size = strtol(vmsize, 0, 0) * 1024;
           break;
        }
    }
    free(line);
    fclose(f);

    return size;
}
