#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>

#define COLOR_DIR "\033[34m"
#define COLOR_EXEC "\033[32m"
#define COLOR_LINK "\033[36m"
#define COLOR_RESET "\033[0m"


int show_all = 0; //параметр -a
int long_format = 0; //параметр -l

typedef struct {
    char name[1024];
    struct stat st;
} FileEntry;

int compare_names(const void *a, const void *b) {
    return strcmp(((FileEntry*)a)->name, ((FileEntry*)b)->name);
}

#ifdef _WIN32
    typedef int uid_t;
    typedef int gid_t;

    // Эмуляция макросов прав доступа для винды, тк они не определены
    #ifndef S_ISLNK
        #define S_ISLNK(mode) 0
    #endif

    #ifndef S_IRGRP
        #define S_IRGRP 0
        #define S_IWGRP 0
        #define S_IXGRP 0
        #define S_IROTH 0
        #define S_IWOTH 0
        #define S_IXOTH 0
    #endif

    // В Windows нет владельцев в юниксовом формате, поэтому всегда возвращаю 0
    char* get_username(uid_t uid) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", 0);
        return buf;
    }

    char* get_groupname(gid_t gid) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", 0);
        return buf;
    }

#else
    // На Linux можно получить имена владельцев
    char* get_username(uid_t uid) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", uid);
        return buf;
    }

    char* get_groupname(gid_t gid) {
        static char buf[32];
        snprintf(buf, sizeof(buf), "%d", gid);
        return buf;
    }
#endif

void format_time(time_t t, char *buf) {
    struct tm *tm = localtime(&t);
    strftime(buf, 20, "%b %d %H:%M", tm);
}

void print_long_entry(FileEntry *entry) {
    char time_buf[20];
    mode_t mode = entry->st.st_mode;

    char perms[11];
    perms[0] = (S_ISDIR(mode)) ? 'd' : (S_ISLNK(mode)) ? 'l' : '-';
    perms[1] = (mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (mode & S_IXUSR) ? 'x' : '-';
    perms[4] = (mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (mode & S_IXGRP) ? 'x' : '-';
    perms[7] = (mode & S_IROTH) ? 'r' : '-';
    perms[8] = (mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (mode & S_IXOTH) ? 'x' : '-';
    perms[10] = '\0';

    char *color = "";
    if (S_ISDIR(mode)) {
        color = COLOR_DIR;
    } else if (S_ISLNK(mode)) {
        color = COLOR_LINK;
    } else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH) {
        color = COLOR_EXEC;
    }

    printf("%s%s %3ld %s %s %8ld ",
           color,
           perms,
           (long)entry->st.st_nlink,
           get_username(entry->st.st_uid),
           get_groupname(entry->st.st_gid),
           (long)entry->st.st_size);

    format_time(entry->st.st_mtime, time_buf);
    printf("%s %s%s\n", time_buf, entry->name, COLOR_RESET);
}

void print_simple_entry(FileEntry *entry) {
    char *color = "";
    mode_t mode = entry->st.st_mode;

    if (S_ISDIR(mode)) {
        color = COLOR_DIR;
    } else if (S_ISLNK(mode)) {
        color = COLOR_LINK;
    } else if (mode & S_IXUSR || mode & S_IXGRP || mode & S_IXOTH) {
        color = COLOR_EXEC;
    }

    printf("%s%s%s ", color, entry->name, COLOR_RESET);
}

int process_directory(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        perror("opendir");
        return 1;
    }

    FileEntry *entries = NULL;
    int count = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;

        FileEntry fe;
        strncpy(fe.name, entry->d_name, sizeof(fe.name) - 1);
        fe.name[sizeof(fe.name) - 1] = '\0';

        char full_path[2048];
#ifdef _WIN32
        snprintf(full_path, sizeof(full_path), "%s\\%s", path, entry->d_name);
#else
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
#endif

        if (stat(full_path, &fe.st) == -1) {
            continue;
        }

        entries = realloc(entries, (count + 1) * sizeof(FileEntry));
        if (!entries) {
            perror("realloc");
            closedir(dir);
            return 1;
        }
        entries[count++] = fe;
    }

    closedir(dir);

    if (count == 0) {
        return 0;
    }

    qsort(entries, count, sizeof(FileEntry), compare_names);

    if (long_format) {
        for (int i = 0; i < count; i++) {
            print_long_entry(&entries[i]);
        }
    } else {
        int max_len = 0;
        for (int i = 0; i < count; i++) {
            int len = strlen(entries[i].name);
            if (len > max_len) max_len = len;
        }
        int col_width = max_len + 2;
        int cols = 80 / col_width;
        if (cols < 1) cols = 1;

        for (int i = 0; i < count; i++) {
            print_simple_entry(&entries[i]);
            if ((i + 1) % cols == 0) printf("\n");
        }
        if (count % cols != 0) printf("\n");
    }

    free(entries);
    return 0;
}

int main(int argc, char *argv[]) {
    show_all = 0;
    long_format = 0;

    int opt;
    while ((opt = getopt(argc, argv, "la")) != -1) {
        switch (opt) {
            case 'l':
                long_format = 1;
                break;
            case 'a':
                show_all = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [-a] [directory]\n", argv[0]);
                exit(1);
        }
    }

    char *path = ".";
    if (optind < argc) {
        path = argv[optind];
    }

    return process_directory(path);
}