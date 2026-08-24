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

typedef struct {
    int PrintPaths;
    int AppendPaths;
    int OmitMatches;
    int Multiline;
    int Format;
    int Debug;
    char InputPathText[1024];
    char FileText[128];
    char OutputPathText[1024];
    char RegularExpressionText[2048];
    char DebugOptions[2048];
    char DelimiterText[16];
} Options;

typedef struct {
    int error;
    char replace_str[1024];
    char CSV_header[1024];
} CSV;

// Function declarations
int ParseExpression(const char *expression, CSV *csv, char *delimiter);
int WriteBuffer(char *line, char **buffer, bool reset);
int CompileCmd(char *cmd, Options options, char **OutputText);
int OpenProcess(FILE **pipe, char *cmd);
int WriteToFile(char *OutputText, char *filename);

#endif // REGEXSEARCHER_H

