#include "regexsearcher.h"
#include <string.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

int ParseExpression(const char *expression, CSV *csv) {
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

        char HeaderBuffer[MAX_LINESIZE] = {0};
        if (found_name) snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%s", found_name);
        else snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%d", i);
        strncat(csv->CSV_header, HeaderBuffer, MAX_LINESIZE);

        char ReplaceBuffer[MAX_LINESIZE];
        snprintf(ReplaceBuffer, sizeof(ReplaceBuffer), "$%d", i);
        strncat(csv->replace_str, ReplaceBuffer, MAX_LINESIZE);

        if (i < capture_count) {
            strncat(csv->CSV_header, csv->delimiter, MAX_LINESIZE);
            strncat(csv->replace_str, csv->delimiter, MAX_LINESIZE);
        }
    }
    return 0;
}

int WriteFormatHeader(Options options, CSV *csv, char **OutputText) {
    // Clear contents of header, otherwise ParseExpression adds to the existing one
    strcpy(csv->CSV_header, "");
    if (ParseExpression(options.RegularExpressionText, csv)) return 1;

    WriteBuffer(csv->CSV_header, OutputText, false);
    WriteBuffer("\n", OutputText, false);
    return 0;
}

int WriteBuffer(char *line, char **buffer, bool reset) {
    static size_t total_size = 0;

    if (reset) {
        if (*buffer != NULL) {
            free(*buffer);
            *buffer = NULL;
        }
        total_size = 0;
        return 0;
    }

    if (line == NULL) return 0;

    size_t line_len = strlen(line);
    // Reallocate memory based on size of string
    char *temp = realloc(*buffer, total_size + line_len + 1);
    if (temp == NULL) {
        perror("realloc failed");
        return 1;
    }
    *buffer = temp;

    // Append the line
    memcpy(*buffer + total_size, line, line_len);
    total_size += line_len;
    (*buffer)[total_size] = '\0'; 
    return 0;
}

int CompileCmd(char *cmd, Options options, CSV *csv) {
    char cmdbuffer[MAX_LINESIZE];

    strncat(cmd, "rg --sort=path ", MAX_LINESIZE);

    if (strcmp(options.FileText, "")) {
        sprintf(cmdbuffer, "-g \"%s\" ", options.FileText);
        strncat(cmd, cmdbuffer, MAX_LINESIZE);
    }

    if (options.PrintPaths) {
        strncat(cmd, "--files ", MAX_LINESIZE);
        return 0;
    }

    if (options.Multiline) {
        strncat(cmd, "-U ", MAX_LINESIZE);
    }

    if (options.LineNumbers) {
        strncat(cmd, "--line-number ", MAX_LINESIZE);
    }

    if (!options.AppendPaths) {
        strncat(cmd, "--no-filename ", MAX_LINESIZE);
    }

    if (options.OmitMatches) {
        strncat(cmd, "--only-matching ", MAX_LINESIZE);
    }

    if (options.Debug) {
        strncat(cmd, options.DebugOptions, MAX_LINESIZE);
    }

    memset(csv, 0, sizeof(CSV));
    strcpy(csv->delimiter, ",");
    if (strcmp(options.DelimiterText, "")) strcpy(csv->delimiter, options.DelimiterText);

    if (options.Format) {
        // Get replace_str from parsing regular expression
        if (ParseExpression(options.RegularExpressionText, csv)) return 1;
#ifdef _WIN32
        sprintf(cmdbuffer, "--replace \"%s\" ", csv->replace_str);
#else
        sprintf(cmdbuffer, "--replace \'%s\' ", csv->replace_str);
#endif
        strncat(cmd, cmdbuffer, MAX_LINESIZE);
    }

    if (strcmp(options.RegularExpressionText, "")) {
        sprintf(cmdbuffer, "\"%s\" ", options.RegularExpressionText);
        strncat(cmd, cmdbuffer, MAX_LINESIZE);
    }

    if (strcmp(options.InputPathText, "")) {
        sprintf(cmdbuffer, "%s ", options.InputPathText);
        strncat(cmd, cmdbuffer, MAX_LINESIZE);
    }

    // make sure stderr is included in stdout
    strncat(cmd, " 2>&1", MAX_LINESIZE);
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

