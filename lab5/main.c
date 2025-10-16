#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <errno.h>
#include <limits.h>

#define HEADER_SIZE 512
#define MAX_FILENAME 256

typedef struct
{
    char filename[MAX_FILENAME];
    off_t file_size;
    mode_t mode;
    time_t mtime;
    time_t atime;
    int is_deleted;
} ArchiveHeader;

void print_help()
{
    printf("Usage:\n");
    printf("  ./archiver arch_name -i(--input) file1\n");
    printf("  ./archiver arch_name -e(--extract) file1\n");
    printf("  ./archiver arch_name -s(--stat)\n");
    printf("  ./archiver -h(--help)\n");
}

int write_header(int fd, const char *filename, off_t size, mode_t mode, time_t mtime, time_t atime, int deleted)
{
    ArchiveHeader hdr = {0};
    strncpy(hdr.filename, filename, MAX_FILENAME - 1);
    hdr.file_size = size;
    hdr.mode = mode;
    hdr.mtime = mtime;
    hdr.atime = atime;
    hdr.is_deleted = deleted;

    if (write(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
    {
        perror("write header");
        return -1;
    }
    return 0;
}

int add_file_to_archive(const char *arch_name, const char *file_name)
{
    int src_fd = open(file_name, O_RDONLY);
    if (src_fd == -1)
    {
        perror("open source file");
        return 1;
    }

    struct stat st;
    if (fstat(src_fd, &st) == -1)
    {
        perror("fstat source");
        close(src_fd);
        return 1;
    }

    int arch_fd = open(arch_name, O_RDWR | O_CREAT, 0644);
    if (arch_fd == -1)
    {
        perror("open archive");
        close(src_fd);
        return 1;
    }

    if (lseek(arch_fd, 0, SEEK_END) == -1)
    {
        perror("lseek archive");
        close(src_fd);
        close(arch_fd);
        return 1;
    }

    if (write_header(arch_fd, file_name, st.st_size, st.st_mode, st.st_mtime, st.st_atime, 0) != 0)
    {
        close(src_fd);
        close(arch_fd);
        return 1;
    }

    char buf[4096];
    ssize_t n;
    off_t total = 0;
    while (total < st.st_size)
    {
        n = read(src_fd, buf, sizeof(buf));
        if (n <= 0)
            break;
        if (write(arch_fd, buf, n) != n)
        {
            perror("write data");
            close(src_fd);
            close(arch_fd);
            return 1;
        }
        total += n;
    }

    close(src_fd);
    close(arch_fd);
    printf("Added '%s' to archive '%s'\n", file_name, arch_name);
    return 0;
}

int extract_file_from_archive(const char *arch_name, const char *file_name)
{
    int arch_fd = open(arch_name, O_RDONLY);
    if (arch_fd == -1)
    {
        perror("open archive");
        return 1;
    }

    ArchiveHeader hdr;
    off_t pos = 0;
    int found = 0;
    char *extracted_data = NULL;
    mode_t extracted_mode = 0;
    time_t extracted_atime = 0, extracted_mtime = 0;
    off_t extracted_size = 0;

    while (1)
    {
        if (lseek(arch_fd, pos, SEEK_SET) == -1)
            break;
        ssize_t n = read(arch_fd, &hdr, sizeof(hdr));
        if (n == 0)
            break;
        if (n != sizeof(hdr))
        {
            fprintf(stderr, "Corrupted header at %ld\n", pos);
            break;
        }

        if (strcmp(hdr.filename, file_name) == 0 && !hdr.is_deleted)
        {
            found = 1;
            extracted_mode = hdr.mode;
            extracted_atime = hdr.atime;
            extracted_mtime = hdr.mtime;
            extracted_size = hdr.file_size;

            if (lseek(arch_fd, pos + sizeof(hdr), SEEK_SET) == -1)
            {
                perror("lseek to data");
                close(arch_fd);
                return 1;
            }

            extracted_data = malloc(extracted_size);
            if (!extracted_data)
            {
                perror("malloc for extraction");
                close(arch_fd);
                return 1;
            }

            ssize_t total = 0;
            while (total < extracted_size)
            {
                ssize_t nread = read(arch_fd, extracted_data + total, extracted_size - total);
                if (nread <= 0)
                    break;
                total += nread;
            }

            if (total != extracted_size)
            {
                fprintf(stderr, "Failed to read full file data\n");
                free(extracted_data);
                close(arch_fd);
                return 1;
            }
            break;
        }

        pos += sizeof(hdr) + hdr.file_size;
    }

    close(arch_fd);

    if (!found)
    {
        fprintf(stderr, "File '%s' not found in archive\n", file_name);
        return 1;
    }

    int out_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, extracted_mode);
    if (out_fd == -1)
    {
        perror("create output file");
        free(extracted_data);
        return 1;
    }

    if (write(out_fd, extracted_data, extracted_size) != extracted_size)
    {
        perror("write output file");
        close(out_fd);
        free(extracted_data);
        return 1;
    }
    close(out_fd);

    struct timespec times[2] = {
        {.tv_sec = extracted_atime, .tv_nsec = 0},
        {.tv_sec = extracted_mtime, .tv_nsec = 0}};
    utimensat(AT_FDCWD, file_name, times, 0);
    chmod(file_name, extracted_mode);

    printf("Extracted '%s' from archive\n", file_name);
    free(extracted_data);

    arch_fd = open(arch_name, O_RDONLY);
    if (arch_fd == -1)
    {
        perror("re-open archive for deletion");
        return 1;
    }

    char temp_name[PATH_MAX];
    snprintf(temp_name, sizeof(temp_name), "%s.tmp.XXXXXX", arch_name);
    int temp_fd = mkstemp(temp_name);
    if (temp_fd == -1)
    {
        perror("create temp archive");
        close(arch_fd);
        return 1;
    }

    pos = 0;
    while (1)
    {
        if (lseek(arch_fd, pos, SEEK_SET) == -1)
            break;
        ssize_t n = read(arch_fd, &hdr, sizeof(hdr));
        if (n == 0)
            break;
        if (n != sizeof(hdr))
        {
            fprintf(stderr, "Corrupted header during rewrite at %ld\n", pos);
            break;
        }

        if (strcmp(hdr.filename, file_name) == 0 && !hdr.is_deleted)
        {
            pos += sizeof(hdr) + hdr.file_size;
            continue;
        }

        if (write(temp_fd, &hdr, sizeof(hdr)) != sizeof(hdr))
        {
            perror("write header to temp archive");
            close(arch_fd);
            close(temp_fd);
            unlink(temp_name);
            return 1;
        }

        if (hdr.file_size > 0)
        {
            char *buf = malloc(hdr.file_size);
            if (!buf)
            {
                perror("malloc for rewrite");
                close(arch_fd);
                close(temp_fd);
                unlink(temp_name);
                return 1;
            }

            if (lseek(arch_fd, pos + sizeof(hdr), SEEK_SET) == -1 ||
                read(arch_fd, buf, hdr.file_size) != hdr.file_size ||
                write(temp_fd, buf, hdr.file_size) != hdr.file_size)
            {
                perror("copy file data during rewrite");
                free(buf);
                close(arch_fd);
                close(temp_fd);
                unlink(temp_name);
                return 1;
            }
            free(buf);
        }

        pos += sizeof(hdr) + hdr.file_size;
    }

    close(arch_fd);
    close(temp_fd);

    if (rename(temp_name, arch_name) != 0)
    {
        perror("replace archive");
        unlink(temp_name);
        return 1;
    }

    printf("File '%s' permanently removed from archive\n", file_name);
    return 0;
}

int show_archive_stat(const char *arch_name)
{
    int arch_fd = open(arch_name, O_RDONLY);
    if (arch_fd == -1)
    {
        perror("open archive");
        return 1;
    }

    ArchiveHeader hdr;
    off_t pos = 0;
    int count = 0;

    printf("Archive '%s' contents:\n", arch_name);
    while (1)
    {
        if (lseek(arch_fd, pos, SEEK_SET) == -1)
            break;
        ssize_t n = read(arch_fd, &hdr, sizeof(hdr));
        if (n == 0)
            break;
        if (n != sizeof(hdr))
        {
            fprintf(stderr, "Corrupted header at %ld\n", pos);
            break;
        }

        if (!hdr.is_deleted)
        {
            printf("  %s (%ld bytes, mode=%o)\n", hdr.filename, (long)hdr.file_size, hdr.mode);
            count++;
        }

        pos += sizeof(hdr) + hdr.file_size;
    }

    printf("Total files: %d\n", count);
    close(arch_fd);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        print_help();
        return 1;
    }

    if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
    {
        print_help();
        return 0;
    }

    if (argc < 3)
    {
        fprintf(stderr, "Not enough arguments\n");
        print_help();
        return 1;
    }

    const char *arch_name = argv[1];
    const char *cmd = argv[2];

    if (strcmp(cmd, "-i") == 0 || strcmp(cmd, "--input") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: %s arch_name -i file\n", argv[0]);
            return 1;
        }
        return add_file_to_archive(arch_name, argv[3]);
    }
    else if (strcmp(cmd, "-e") == 0 || strcmp(cmd, "--extract") == 0)
    {
        if (argc != 4)
        {
            fprintf(stderr, "Usage: %s arch_name -e file\n", argv[0]);
            return 1;
        }
        return extract_file_from_archive(arch_name, argv[3]);
    }
    else if (strcmp(cmd, "-s") == 0 || strcmp(cmd, "--stat") == 0)
    {
        if (argc != 3)
        {
            fprintf(stderr, "Usage: %s arch_name -s\n", argv[0]);
            return 1;
        }
        return show_archive_stat(arch_name);
    }
    else
    {
        fprintf(stderr, "Unknown command: %s\n", cmd);
        print_help();
        return 1;
    }
}