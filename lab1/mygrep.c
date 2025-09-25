#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 4096

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s pattern [file...]\n", argv[0]);
        exit(1);
    }

    char *pattern = argv[1];
    int file_count = argc - 2;

    if (file_count == 0)
    {
        char line[MAX_LINE_LENGTH];
        long line_num = 0;
        while (fgets(line, MAX_LINE_LENGTH, stdin))
        {
            line_num++;
            if (strstr(line, pattern) != NULL)
            {
                printf("%ld:%s", line_num, line);
            }
        }
    }
    else
    {
        for (int i = 2; i < argc; i++)
        {
            FILE *fp = fopen(argv[i], "r");
            if (!fp)
            {
                fprintf(stderr, "%s: cannot open file %s\n", argv[0], argv[i]);
                continue;
            }

            char line[MAX_LINE_LENGTH];
            long line_num = 0;
            while (fgets(line, MAX_LINE_LENGTH, fp))
            {
                line_num++;
                if (strstr(line, pattern) != NULL)
                {
                    if (file_count > 1)
                    {
                        printf("%s:", argv[i]);
                    }
                    printf("%ld:%s", line_num, line);
                }
            }

            fclose(fp);
        }
    }

    return 0;
}