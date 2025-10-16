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

            int out_fd = open(file_name, O_WRONLY | O_CREAT | O_TRUNC, hdr.mode);
            if (out_fd == -1)
            {
                perror("create output file");
                close(arch_fd);
                return 1;
            }

            if (lseek(arch_fd, pos + sizeof(hdr), SEEK_SET) == -1)
            {
                close(out_fd);
                close(arch_fd);
                return 1;
            }

            char *buf = malloc(hdr.file_size);
            if (!buf)
            {
                perror("malloc");
                close(out_fd);
                close(arch_fd);
                return 1;
            }

            ssize_t total = 0;
            while (total < hdr.file_size)
            {
                ssize_t n = read(arch_fd, buf + total, hdr.file_size - total);
                if (n <= 0)
                    break;
                total += n;
            }

            if (write(out_fd, buf, total) != total)
            {
                perror("write output file");
                free(buf);
                close(out_fd);
                close(arch_fd);
                return 1;
            }
            free(buf);
            close(out_fd);

            struct timespec times[2] = {
                {.tv_sec = hdr.atime, .tv_nsec = 0},
                {.tv_sec = hdr.mtime, .tv_nsec = 0}};
            utimensat(AT_FDCWD, file_name, times, 0);
            chmod(file_name, hdr.mode);

            printf("Extracted '%s' from archive\n", file_name);
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