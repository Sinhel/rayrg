#ifndef REGEXSEARCHER_H
#define REGEXSEARCHER_H

#include <stdio.h>
#include <stdbool.h>

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#else
#define POPEN popen
#define PCLOSE pclose
#endif //_WIN32

#define MAX_LINESIZE 2048
#define DELIMITER 16

typedef struct {
    int PrintPaths;
    int AppendPaths;
    int OmitMatches;
    int Multiline;
    int Format;
    int Debug;
    char InputPathText[MAX_LINESIZE];
    char FileText[MAX_LINESIZE];
    char OutputPathText[MAX_LINESIZE];
    char RegularExpressionText[MAX_LINESIZE];
    char DebugOptions[MAX_LINESIZE];
    char DelimiterText[DELIMITER];
} Options;

typedef struct {
    int error;
    char replace_str[MAX_LINESIZE];
    char CSV_header[MAX_LINESIZE];
    char delimiter[DELIMITER];
} CSV;

// Function declarations
int ParseExpression(const char *expression, CSV *csv, char *delimiter);
int WriteBuffer(char *line, char **buffer, bool reset);
int CompileCmd(char *cmd, Options options, char **OutputText);
int OpenProcess(FILE **pipe, char *cmd);
int WriteToFile(char *OutputText, char *filename);

#endif // REGEXSEARCHER_H

