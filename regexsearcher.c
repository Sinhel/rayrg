#include "regexsearcher.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <string.h>

#define LINESIZE 2048

int ParseExpression(const char *expression, CSV *csv, char *delimiter) {
    int errornumber;
    PCRE2_SIZE erroroffset;
    pcre2_code *re = pcre2_compile((PCRE2_SPTR)expression, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, NULL);
    if (!re) return 1;

    int capture_count;
    int name_count;
    PCRE2_SPTR name_table;
    int name_entry_size;
    pcre2_pattern_info(re, PCRE2_INFO_CAPTURECOUNT, &capture_count);
    pcre2_pattern_info(re, PCRE2_INFO_NAMECOUNT, &name_count);
    pcre2_pattern_info(re, PCRE2_INFO_NAMETABLE, &name_table);
    pcre2_pattern_info(re, PCRE2_INFO_NAMEENTRYSIZE, &name_entry_size);

    for (int i = 1; i <= capture_count; i++) {
        const char *found_name = NULL;

        for (int j = 0; j < name_count; j++) {
            PCRE2_SPTR entry = name_table + j * name_entry_size;
            int group_index = (entry[0] << 8) | entry[1];
            if (group_index == i) {
                found_name = (const char *)(entry + 2);
                break;
            }
        }

        char HeaderBuffer[128] = {0};
        if (found_name) snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%s", found_name);
        else snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%d", i);
        strcat(csv->CSV_header, HeaderBuffer);

        char ReplaceBuffer[128];
        snprintf(ReplaceBuffer, sizeof(ReplaceBuffer), "$%d", i);
        strcat(csv->replace_str, ReplaceBuffer);

        if (i < capture_count) {
            strcat(csv->CSV_header, delimiter);
            strcat(csv->replace_str, delimiter);
        }
    }
    return 0;
}

int WriteBuffer(char *line, char **buffer, bool reset) {
    static size_t total_size = 0;

    if (reset) {
        if (*buffer != NULL) memset(*buffer, 0, total_size);
        total_size = 0;
        return 0;
    }

    if (line == NULL) return 0;

    size_t line_len = strlen(line);
    // Reallocate memory based on size of string
    char *temp = realloc(*buffer, total_size + line_len + 1);
    if (temp == NULL) {
        perror("pclose failed");
        return 1;
    }
    *buffer = temp;

    // Append the line
    memcpy(*buffer + total_size, line, line_len);
    total_size += line_len;
    return 0;
}

int CompileCmd(char *cmd, Options options, char **OutputText) {
    char cmdbuffer[4096];

    strcat(cmd, "rg --sort=path ");

    if (strcmp(options.FileText, "")) {
        sprintf(cmdbuffer, "-g \"%s\" ", options.FileText);
        strcat(cmd, cmdbuffer);
    }

    if (options.PrintPaths) {
        strcat(cmd, "--files ");
        return 0;
    }

    if (options.Multiline) {
        strcat(cmd, "-U ");
    }

    if (!options.AppendPaths) {
        strcat(cmd, "--no-filename ");
    }

    if (options.OmitMatches) {
        strcat(cmd, "--only-matching ");
    }

    if (options.Debug) {
        strcat(cmd, options.DebugOptions);
    }

    char delimiter[16] = ",";
    if (strcmp(options.DelimiterText, "")) strcpy(delimiter, options.DelimiterText);

    CSV csv = {0};
    if (options.Format) {
        if (ParseExpression(options.RegularExpressionText, &csv, delimiter)) return 1;
        if (options.AppendPaths) WriteBuffer("Filename,", OutputText, false);

        WriteBuffer(csv.CSV_header, OutputText, false);
        WriteBuffer("\n", OutputText, false);
#ifdef _WIN32
        sprintf(cmdbuffer, "--replace \"%s\" ", csv.replace_str);
#else
        sprintf(cmdbuffer, "--replace \'%s\' ", csv.replace_str);
#endif
        strcat(cmd, cmdbuffer);
    }

    if (strcmp(options.RegularExpressionText, "")) {
        sprintf(cmdbuffer, "\"%s\" ", options.RegularExpressionText);
        strcat(cmd, cmdbuffer);
    }

    if (strcmp(options.InputPathText, "")) {
        sprintf(cmdbuffer, "%s ", options.InputPathText);
        strcat(cmd, options.InputPathText);
    }

    // make sure stderr is included in stdout
    strcat(cmd, " 2>&1");
    return 0;
}

int OpenProcess(FILE **pipe, char *cmd) {
    *pipe = POPEN(cmd, "r");
    if (*pipe == NULL) {
        perror("popen failed");
        return 1;
    }
    return 0;
}

int WriteToFile(char *OutputText, char *filename) {

    FILE *output;
    if (filename == NULL) {
        filename = "output.txt";
    }
    output = fopen(filename, "w");

    if (output == NULL) {
        perror("Failed to file for output");
        return 1;
    }

    fputs(OutputText, output);
    fclose(output);
    printf("Data written to %s\n", filename);
    return 0;
}
