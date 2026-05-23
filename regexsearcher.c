/*******************************************************************************************
 *
 *   Regex searcher v1.0.0 - Calls ripgrep from path.
 *
 *******************************************************************************************/

#ifdef _WIN32
#define POPEN _popen
#define PCLOSE _pclose
#define PCRE2_STATIC
#else
#define POPEN popen
#define PCLOSE pclose
#endif //_WIN32

#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JetBrainsMono-Medium.h"

#define LINESIZE 2048

typedef struct {
    bool PrintPaths;
    bool AppendPaths;
    bool OmitMatches;
    bool Multiline;
    bool Format;
    char InputPathText[1024];
    char FileText[128];
    char OutputPathText[1024];
    char RegularExpressionText[2048];
} Options;

typedef struct {
    int error;
    char replace_str[1024];
    char CSV_header[1024];
} CSV;

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------

int ParseExpression(const char *expression, CSV *csv) {
    int errornumber;
    PCRE2_SIZE erroroffset;
    pcre2_code *re = pcre2_compile((PCRE2_SPTR)expression, PCRE2_ZERO_TERMINATED, 0, &errornumber, &erroroffset, NULL);
    if (!re)
        return 1;

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
        if (found_name)
            snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%s", found_name);
        else
            snprintf(HeaderBuffer, sizeof(HeaderBuffer), "%d", i);
        strcat(csv->CSV_header, HeaderBuffer);

        char ReplaceBuffer[128];
        snprintf(ReplaceBuffer, sizeof(ReplaceBuffer), "$%d", i);
        strcat(csv->replace_str, ReplaceBuffer);
        if (i < capture_count) {
            strcat(csv->CSV_header, ",");
            strcat(csv->replace_str, ",");
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
    CSV csv = {0};

    strcat(cmd, "rg ");
    if (options.PrintPaths) {
        strcat(cmd, "--files ");
    }
    if (options.Multiline) {
        strcat(cmd, "-U ");
    }

    if (strcmp(options.FileText, "")) {
        sprintf(cmdbuffer, "-g \"%s\" ", options.FileText);
        strcat(cmd, cmdbuffer);
    }

    if (!options.AppendPaths) {
        strcat(cmd, "--no-filename ");
    }
    if (options.OmitMatches) {
        strcat(cmd, "--only-matching ");
    }

    if (options.Format) {
        if (ParseExpression(options.RegularExpressionText, &csv))
            return 1;
        if (options.AppendPaths) WriteBuffer("Filename,", OutputText, false);
        WriteBuffer(csv.CSV_header, OutputText, false);
        WriteBuffer("\n", OutputText, false);
        sprintf(cmdbuffer, "--replace \"%s\" ", csv.replace_str);
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

    // TODO remove
    RAYGUI_LOG("cmd = %s\n", cmd);
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
    RAYGUI_LOG("Data written to %s\n", filename);
    return 0;
}

//------------------------------------------------------------------------------------
// Program main entry point
//------------------------------------------------------------------------------------
int main() {
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = 904;
    int screenHeight = 568;

    InitWindow(screenWidth, screenHeight, "Regex searcher");
    SetExitKey(KEY_NULL); // Disables the ESC key from triggering WindowShouldClose()

    // Needed to load æøåÆØÅ
    const char *CharSet = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~øæåØÆÅ";
    int CharCount = 0;
    int *CharCodepoints = LoadCodepoints(CharSet, &CharCount);

    Font JetBrainsMono = LoadFontFromMemory(".ttf", JetBrainsMono_Medium_ttf, JetBrainsMono_Medium_ttf_len, 32, CharCodepoints, CharCount);
    GuiSetFont(JetBrainsMono);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    // Regex searcher: controls initialization
    //----------------------------------------------------------------------------------
    bool WindowActive = true;
    bool InputPathEditMode = false;
    bool FileEditMode = false;
    bool RegularExpressionEditMode = false;
    bool OutputPathEditMode = false;
    bool ButtonSearch = false;

    char *OutputText = malloc(1024 * 8);
    strcpy(OutputText, "Output");

    Vector2 TextSize = {0};

    Rectangle ScrollOutputScrollView = {0, 0, 0, 0};
    Vector2 ScrollOutputScrollOffset = {0, 0};

    FILE *pipe = {0};
    int reading = 0;
    char line[LINESIZE] = {0};
    char cmd[LINESIZE] = {0};
    Options options = {0};

    SetTargetFPS(60);

    // Main loop
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------

        // Read the output into the line buffer
        if (reading) {
            // Do 1000 reads per frame, to not slow down reading too much
            // (possible to do 60.000 lines per second with this loop)
            for (int i = 0; i < 1000; i++) {
                if (fgets(line, LINESIZE, pipe) != NULL) {
                    reading = 1;
                } else {
                    reading = 0;
                    int status = PCLOSE(pipe);
                    if (status == -1) {
                        return 1;
                    }
                    // cleanup and leave loop
                    if (strcmp(options.OutputPathText, "")) {
                        WriteToFile(OutputText, options.OutputPathText);
                    }
                    break;
                }
                if (WriteBuffer(line, &OutputText, false)) {
                }
            }
        }
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        // raygui: controls drawing
        //----------------------------------------------------------------------------------
        if (WindowActive) {
            WindowActive = !GuiGroupBox((Rectangle){0, 0, 1200, 800}, NULL);
            if (GuiTextBox((Rectangle){24, 16, 496, 24}, options.InputPathText, 128, InputPathEditMode))
                InputPathEditMode = !InputPathEditMode;
            if (GuiTextBox((Rectangle){24, 56, 496, 24}, options.FileText, 128, FileEditMode))
                FileEditMode = !FileEditMode;
            if (GuiTextBox((Rectangle){24, 96, 496, 24}, options.OutputPathText, 128, OutputPathEditMode))
                OutputPathEditMode = !OutputPathEditMode;
            if (GuiTextBox((Rectangle){24, 136, 496, 24}, options.RegularExpressionText, 128, RegularExpressionEditMode))
                RegularExpressionEditMode = !RegularExpressionEditMode;
            GuiGroupBox((Rectangle){544, 8, 304, 200}, "Options");
            GuiCheckBox((Rectangle){568, 16, 24, 24}, "Print paths", &options.PrintPaths);
            GuiCheckBox((Rectangle){568, 56, 24, 24}, "AppendPaths", &options.AppendPaths);
            GuiCheckBox((Rectangle){568, 96, 24, 24}, "OmitMatches", &options.OmitMatches);
            GuiCheckBox((Rectangle){568, 136, 24, 24}, "Multiline", &options.Multiline);
            GuiCheckBox((Rectangle){568, 176, 24, 24}, "Format", &options.Format);
            ButtonSearch = GuiButton((Rectangle){24, 176, 496, 24}, "Search");
            TextSize = MeasureTextEx(JetBrainsMono, OutputText, GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING));
            GuiScrollPanel((Rectangle){24, 216, 864, 350}, NULL, (Rectangle){80, 320, TextSize.x, TextSize.y}, &ScrollOutputScrollOffset,
                           &ScrollOutputScrollView);

            // Draw default values in textboxes
            {
                if (!strcmp(options.InputPathText, "")) {
                    DrawTextEx(JetBrainsMono, "InputPath", (Vector2){28, 19}, GuiGetStyle(DEFAULT, TEXT_SIZE),
                               GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                }
                if (!strcmp(options.FileText, "")) {
                    DrawTextEx(JetBrainsMono, "File", (Vector2){28, 59}, GuiGetStyle(DEFAULT, TEXT_SIZE),
                               GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                }
                if (!strcmp(options.OutputPathText, "")) {
                    DrawTextEx(JetBrainsMono, "OutputPath", (Vector2){28, 99}, GuiGetStyle(DEFAULT, TEXT_SIZE),
                               GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                }
                if (!strcmp(options.RegularExpressionText, "")) {
                    DrawTextEx(JetBrainsMono, "RegularExpression", (Vector2){28, 139}, GuiGetStyle(DEFAULT, TEXT_SIZE),
                               GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                }
            }
            // Draw content in GuiScrollPanel
            BeginScissorMode(24, 216, 864, 350);
            {
                DrawTextEx(JetBrainsMono, OutputText, (Vector2){24 + ScrollOutputScrollOffset.x, 216 + ScrollOutputScrollOffset.y},
                           GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING),
                           GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
            }
            EndScissorMode();

            //----------------------------------------------------------------------------------
        }
        EndDrawing();
        //----------------------------------------------------------------------------------
        //Tie formatting option to always omit full matches
        if (options.Format) options.OmitMatches = true;

        if (ButtonSearch || IsKeyPressed(KEY_ENTER)) {
            WriteBuffer(NULL, &OutputText, true);
            strcpy(cmd, "");
            CompileCmd(cmd, options, &OutputText);
            if (!OpenProcess(&pipe, cmd)) {
                reading = 1;
            }
        }
    }

    // De-Initialization
    //---------------------------------------------------------------------------------------
    CloseWindow();

    return 0;
}
