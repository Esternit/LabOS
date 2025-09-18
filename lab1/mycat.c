#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 1024

void process_line(char *line, int *line_num, int show_numbers, int show_nonempty_only, int show_eol)
{
    size_t len = strlen(line);
    int is_empty = (len == 1 && line[0] == '\n');

    if (show_nonempty_only && is_empty)
    {
        if (show_eol && len > 0 && line[len - 1] == '\n')
        {
            line[len - 1] = '\0';
            printf("%s$\n", line);
        }
        else if (show_eol)
        {
            printf("%s$", line);
        }
        else
        {
            fputs(line, stdout);
        }
        return;
    }

    if (show_numbers)
    {
        printf("%6d\t", (*line_num)++);
    }

    if (show_eol && len > 0 && line[len - 1] == '\n')
    {
        line[len - 1] = '\0';
        printf("%s$\n", line);
    }
    else if (show_eol)
    {
        printf("%s$", line);
    }
    else
    {
        fputs(line, stdout);
    }
}

int main(int argc, char *argv[])
{
    int show_numbers = 0;       // Это для флага n
    int show_nonempty_only = 0; // Это для флага b
    int show_eol = 0;           // Это для флага E

    int i = 1;
    while (i < argc && argv[i][0] == '-')
    {
        char *flag = argv[i];
        for (int j = 1; flag[j] != '\0'; j++)
        {
            switch (flag[j])
            {
            case 'n':
                show_numbers = 1;
                break;
            case 'b':
                show_nonempty_only = 1;
                show_numbers = 1;
                break;
            case 'E':
                show_eol = 1;
                break;
            default:
                break;
            }
        }
        i++;
    }

    if (i >= argc)
    {
        if (isatty(STDIN_FILENO))
        {
            fprintf(stderr, "mycat: waiting for input from stdin... (Press Ctrl+D to end)\n");
        }

        char line[MAX_LINE_LENGTH];
        int line_num = 1;

        while (fgets(line, MAX_LINE_LENGTH, stdin))
        {
            process_line(line, &line_num, show_numbers, show_nonempty_only, show_eol);
        }
    }
    else
    {
        for (; i < argc; i++)
        {
            FILE *fp = fopen(argv[i], "r");
            if (!fp)
            {
                fprintf(stderr, "mycat: cannot open file %s\n", argv[i]);
                exit(1);
            }

            char line[MAX_LINE_LENGTH];
            int line_num = 1;

            while (fgets(line, MAX_LINE_LENGTH, fp))
            {
                process_line(line, &line_num, show_numbers, show_nonempty_only, show_eol);
            }

            fclose(fp);
        }
    }

    return 0;
}