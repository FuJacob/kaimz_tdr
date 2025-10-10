#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(void) {
    const char *log_paths[] = {
        "/var/log/syslog",     // Debian/Ubuntu
        "/var/log/messages"    // CentOS/Fedora
    };

    FILE *fp = NULL;
    char buffer[1024];

    // Try opening whichever log file exists
    for (int i = 0; i < 2; i++) {
        fp = fopen(log_paths[i], "r");
        if (fp != NULL) {
            printf("Following system log: %s\n\n", log_paths[i]);
            break;
        }
    }

    if (fp == NULL) {
        perror("Error opening system log file");
        return 1;
    }

    // Seek to end of file (like tail -f)
    fseek(fp, 0, SEEK_END);

    while (1) {
        // Try to read a new line
        if (fgets(buffer, sizeof(buffer), fp) != NULL) {
            printf("%s", buffer);
            fflush(stdout);
        } else {
            // No new data yet — wait a bit
            clearerr(fp);
            usleep(500000); // 0.5 seconds
        }
    }

    fclose(fp);
    return 0;
}

