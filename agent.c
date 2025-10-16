#define _GNU_SOURCE
#include <curl/curl.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <unistd.h>

#define LINE_BUF 4096
#define EVENT_BUF_LEN (1024 * (sizeof(struct inotify_event) + 16))

/* CONFIG — change or read from a file/env in real agent */
const char* log_candidates[] = { "/var/log/syslog", "/var/log/messages" };
const char* server_url = "https://example.com/ingest"; /* replace with your endpoint */
const char* auth_token = "REPLACE_WITH_TOKEN"; /* optional auth */

static volatile sig_atomic_t keep_running = 1;

static void handle_sig(int sig)
{
    (void)sig;
    keep_running = 0;
}

static FILE* open_follow(const char* path)
{
    FILE* fp = fopen(path, "r");
    if (!fp)
        return NULL;
    if (fseeko(fp, 0, SEEK_END) != 0) {
        fclose(fp);
        return NULL;
    }
    return fp;
}

static int send_line_to_server(CURL* curl, const char* line)
{
    struct curl_slist* hdrs = NULL;
    int rc = 0;
    char auth_hdr[256];

    /* build auth header if token present */
    if (auth_token && auth_token[0]) {
        snprintf(auth_hdr, sizeof(auth_hdr), "Authorization: Bearer %s", auth_token);
        hdrs = curl_slist_append(hdrs, auth_hdr);
    }
    hdrs = curl_slist_append(hdrs, "Content-Type: text/plain; charset=utf-8");

    curl_easy_setopt(curl, CURLOPT_URL, server_url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, line);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(line));
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L); /* short timeout for demo */

    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "curl perform failed: %s\n", curl_easy_strerror(res));
        rc = -1;
    }

    curl_slist_free_all(hdrs);
    return rc;
}

int main(void)
{
    const char* path = NULL;
    FILE* fp = NULL;
    int inotify_fd = -1;
    int wd = -1;
    char line[LINE_BUF];

    /* pick first readable log path */
    for (size_t i = 0; i < sizeof(log_candidates) / sizeof(log_candidates[0]); ++i) {
        if (access(log_candidates[i], R_OK) == 0) {
            path = log_candidates[i];
            break;
        }
    }
    if (!path) {
        fprintf(stderr, "No readable log file found.\n");
        return 1;
    }
    printf("Agent will follow: %s\n", path);

    /* signal handlers for graceful shutdown */
    signal(SIGINT, handle_sig);
    signal(SIGTERM, handle_sig);

    /* init libcurl once */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "Failed to init curl\n");
        return 1;
    }

    /* open and seek to end */
    fp = open_follow(path);
    if (!fp) {
        perror("open_follow");
        /* continue: maybe file appears later */
    }

    /* inotify */
    inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init1");
        inotify_fd = -1; /* fallback to polling */
    } else {
        wd = inotify_add_watch(inotify_fd, path, IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF | IN_ATTRIB);
        if (wd < 0) {
            perror("inotify_add_watch");
            wd = -1;
        }
    }

    while (keep_running) {
        /* read any new lines */
        if (fp) {
            while (fgets(line, sizeof(line), fp) != NULL) {
                /* Safe local printing for debug — do NOT pass line as format string */
                fputs(line, stdout);
                fflush(stdout);

                /* forward to server (synchronous demo) */
                if (send_line_to_server(curl, line) != 0) {
                    /* On failure you could enqueue the line to disk or memory for retry.
                       For simplicity we just print an error. */
                    fprintf(stderr, "Failed to send line, will continue.\n");
                }
            }
            clearerr(fp);
        }

        /* if we have inotify, read events */
        if (inotify_fd >= 0) {
            char evbuf[EVENT_BUF_LEN];
            ssize_t r = read(inotify_fd, evbuf, sizeof(evbuf));
            if (r > 0) {
                ssize_t i = 0;
                while (i < r) {
                    struct inotify_event* ev = (struct inotify_event*)(evbuf + i);
                    if (ev->mask & (IN_MOVE_SELF | IN_DELETE_SELF)) {
                        /* file rotated or removed — reopen */
                        if (fp) {
                            fclose(fp);
                            fp = NULL;
                        }
                        /* try reopen immediately */
                        fp = open_follow(path);
                        if (wd >= 0) {
                            inotify_rm_watch(inotify_fd, wd);
                            wd = -1;
                        }
                        if (inotify_fd >= 0)
                            wd = inotify_add_watch(inotify_fd, path, IN_MODIFY | IN_MOVE_SELF | IN_DELETE_SELF | IN_ATTRIB);
                    } else if (ev->mask & IN_ATTRIB) {
                        /* possible truncation — if file shrank, seek to end */
                        if (fp) {
                            struct stat st;
                            int fd = fileno(fp);
                            if (fd >= 0 && fstat(fd, &st) == 0) {
                                off_t off = ftello(fp);
                                if (off > st.st_size) {
                                    fseeko(fp, 0, SEEK_END);
                                }
                            }
                        }
                    }
                    i += sizeof(struct inotify_event) + ev->len;
                }
                continue;
            } else if (r < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("read(inotify_fd)");
            }
        }

        /* fallback sleep when nothing happened */
        usleep(200000);
    }

    /* cleanup */
    if (fp)
        fclose(fp);
    if (wd >= 0 && inotify_fd >= 0)
        inotify_rm_watch(inotify_fd, wd);
    if (inotify_fd >= 0)
        close(inotify_fd);
    curl_easy_cleanup(curl);
    curl_global_cleanup();
    printf("Agent exiting cleanly.\n");
    return 0;
}
