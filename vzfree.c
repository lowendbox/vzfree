#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct resource_t {
    char name[32];
    long long held;
    long long maxheld;
    long long barrier;
    long long limit;
    long long failcnt;
    struct resource_t *next;
}; 


struct resource_t *
get_resource(char* buffer) {
    struct resource_t *resource;
    char *buffer2 = buffer;
    char buffer3[80];
    int state = 0;
    int count = 0;

    resource = malloc(sizeof(struct resource_t));
    resource->next = NULL;

    while (1) {
        switch (state) {
            case 0:
                if (*buffer2 == ' ') {
                    strncpy(resource->name, buffer, (int) (buffer2 - buffer));
                    resource->name[(int) (buffer2 - buffer)] = '\0';
                    state = 1;
                }
                break;
            case 1:
                if (*buffer2 != ' ') {
                    buffer = buffer2;
                    state = 2;
                }
                break;
            case 2:
                if (*buffer2 == ' ' || !*buffer2) {
                    if (count < 5) {
                        sscanf(buffer, "%lld", (&(resource->held) + count));
                        count ++;
                    }
                    state = 1;
                }
                break;
        }
        if (!*buffer2) {
            break;
        }
        buffer2 ++;
    }

    if (strcmp(resource->name, "dummy") == 0) {
        free(resource);
        return NULL;
    } else if (count == 5) {
        return resource;
    } else {
        free(resource);
        return NULL;
    }
}

struct resource_t*
find_resource(struct resource_t* resource, char* name) {
    while (resource) {
        if (strcmp(name, resource->name) == 0) {
            return resource;
        } else {
            resource = resource->next;
        }
    }
    return resource;
}

void
print_memory(struct resource_t *resource) {
    struct resource_t *kmemsize, *privvmpages, *physpages, 
                      *oomguarpages, *vmguarpages;

    float committed_total;
    float committed_used;
    float pagesize; /* pagesize in mega bytes */

    kmemsize = find_resource(resource, "kmemsize");
    privvmpages = find_resource(resource, "privvmpages");
    physpages = find_resource(resource, "physpages");
    oomguarpages = find_resource(resource, "oomguarpages");
    vmguarpages = find_resource(resource, "vmguarpages");
    pagesize = (float) sysconf(_SC_PAGESIZE) / 1048576.0;

    committed_total = oomguarpages->barrier * pagesize;
    committed_used = (oomguarpages->held * pagesize) + (kmemsize->held / 1048576.0);

    printf("               Total       Used       Free\n");
    printf("Kernel:   %9.2fM %9.2fM %9.2fM\n",
           kmemsize->barrier  / 1048576.0,
           kmemsize->held / 1048576.0,
           (kmemsize->barrier - kmemsize->held) / 1048576.0);
    printf("Allocate: %9.2fM %9.2fM %9.2fM (%lldM Guaranteed)\n",
           privvmpages->barrier * pagesize,
           privvmpages->held * pagesize,
           (privvmpages->barrier - privvmpages->held) * pagesize,
           (long long int) (vmguarpages->barrier * pagesize));
    printf("Commit:   %9.2fM %9.2fM %9.2fM (%.1f%% of Allocated)\n",
           committed_total,
           committed_used,
           committed_total - committed_used,
           oomguarpages->held * 100.0 / privvmpages->held);
    printf("Swap:                %9.2fM            (%.1f%% of Committed)\n",
           (oomguarpages->held - physpages->held) * pagesize,
           (oomguarpages->held - physpages->held) / 
            (float) (oomguarpages->held) * 100.0);
}

struct resource_t *
get_meminfo(char* beanfile) {
    FILE *fp;
    char buffer[256];
    char* tmpchar;
    int state = 0, offset = 0;
    struct resource_t *resource = NULL, *resource2, *resource3;

    if (!( fp = fopen(beanfile, "r"))) {
        return 0;
    }

    while (fgets(buffer, 256, fp)) {
        switch (state) {
            case 0:
                if (strncasecmp(buffer, "Version:", 8) == 0) {
                    state = 1;
                }
                break;
            case 1:
                tmpchar = strstr(buffer, "resource");
                if (tmpchar) {
                    offset = (int) (tmpchar - buffer);
                    state = 2;
                }
                break;
            case 2:
                if (resource2 = get_resource((char*) (buffer + offset))) {
                    if (resource) {
                        resource3->next = resource2;
                        resource3 = resource2;
                    } else {
                        resource = resource3 = resource2;
                    }
                }
                break;
        }
    }

    return resource;
}

int
main(int argc, char **argv) {
    struct resource_t* resource;
    char* beanfile;
    if (argc > 1) {
        beanfile = argv[1];
    } else {
        beanfile = "/proc/user_beancounters";
    }
    if (resource = get_meminfo(beanfile)) {
        print_memory(resource);
        return 0;
    } else {
        fprintf(stderr, "Unable to read %s\n", beanfile);
        return -1;
    }
}
