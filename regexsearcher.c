/*******************************************************************************************
*
*   Regex searcher v1.0.0 - Calls ripgrep from path. 
*
**********************************************************************************************/

#include "raylib.h"

#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define POPEN _popen
    #define PCLOSE _pclose
#else
    #define POPEN popen
    #define PCLOSE pclose
#endif

#define LINESIZE 2048

typedef struct {
    bool PrintPaths;
    bool FilterTypes;
    bool AppendPaths;
    bool OmitMatches;
    char InputPathText[1024];
    char FileTypeText[128];
    char *OutputPathText;
    char *RegularExpressionText;
} Options;

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------


void CompileCmd(char *cmd, Options options) {
    char buffer[1024];

    strcat(cmd, "rg ");
    printf("filetype to filer: %s\n", options.FileTypeText);
    if (options.PrintPaths)  {strcat(cmd, "--files ");}
    if (options.FilterTypes) sprintf(buffer, "%s ", options.FileTypeText); strcat(cmd, buffer); 
    if (options.AppendPaths) {strcat(cmd, "--with-filename ");}
    if (options.OmitMatches) {strcat(cmd, "--only-matching ");}
    strcat(cmd, options.InputPathText);

    //make sure stderr is included in stdout
    strcat(cmd, " 2>&1");
    RAYGUI_LOG("cmd = %s\n", cmd);
}

int OpenProcess(FILE **pipe, char *cmd)
{
    *pipe = POPEN(cmd, "r");
    if (*pipe == NULL) {
        perror("popen failed");
        return 1;
    }
    return 0;
}

int ReadProcess(FILE *pipe, char *line, char **buffer, size_t *total_size) {
    if (fgets(line, LINESIZE, pipe) != NULL) {
        size_t line_len = strlen(line);

        // Reallocate memory for new line
        char *temp = realloc(*buffer, *total_size + line_len + 1);
        if (temp == NULL) {
            perror("realloc failed");
            free(*buffer);
            PCLOSE(pipe);
            return 1;
        }
        *buffer = temp;

        // Append the line
        strcpy(*buffer + *total_size, line);
        *total_size += line_len;
        return 0;
    }
    //RAYGUI_LOG("ReadProcess output: %s", *buffer); TODO
    return 1;
}

int WriteToFile(char* OutputText, char * filename) {

    FILE *output;
    if (filename == NULL) {filename = "output.txt";}
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
int main()
{
    // Initialization
    //---------------------------------------------------------------------------------------
    int screenWidth = 904;
    int screenHeight = 568;


    InitWindow(screenWidth, screenHeight, "Regex searcher");
    Font JetBrainsMono = LoadFont("./JetBrainsMono-Medium.ttf");
    GuiSetFont(JetBrainsMono);
    GuiSetStyle(DEFAULT, TEXT_SIZE, 16);

    // Regex searcher: controls initialization
    //----------------------------------------------------------------------------------
    bool WindowActive = true;
    bool InputPathEditMode = false;
    bool FileTypeEditMode = false;
    bool RegularExpressionEditMode = false;
    char RegularExpressionText[128] = "RegularExpression";
    bool OutputPathEditMode = false;
    char OutputPathText[128] = "OutputPath";
    bool OutputEditMode = false;
    bool ButtonSearch = false;

    char *OutputText = malloc(1024*8);
    strcpy(OutputText, "Output");

    Vector2 TextSize = {0};

    Rectangle ScrollOutputScrollView = {0, 0, 0, 0};
    Vector2 ScrollOutputScrollOffset =  {0, 0};
    Vector2 ScrollOutputBoundsOffset =  {0, 0};

    FILE *pipe = {0};
    int reading = 0;
    size_t total_size = 0;
    char line[LINESIZE] = {0};
    char cmd[LINESIZE] = {0};
    Options options = {0};
    strcpy(options.InputPathText, "InputPath");
    strcpy(options.FileTypeText, "Filetype");
    //----------------------------------------------------------------------------------
    SetTargetFPS(60);
    //--------------------------------------------------------------------------------------

    // Main loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // Update
        //----------------------------------------------------------------------------------
        
        // Read the output into the line buffer
            if (reading) {
                for (int i = 0; i < 1000; i++) {
                    if (!ReadProcess(pipe, line, &OutputText, &total_size)) reading = 1;
                    else { 
                        reading = 0; break;

                        int status = PCLOSE(pipe);
                        if (status == -1) {
                            perror("pclose failed");
                            return 1;
                        }
                        strcpy(cmd, "");
                        printf("Process closed\n");
                    }
                }
            }
        // Draw
        //----------------------------------------------------------------------------------
        BeginDrawing();

            ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR))); 

            // raygui: controls drawing
            //----------------------------------------------------------------------------------
            if (WindowActive)
            {
                //WindowActive = !GuiWindowBox((Rectangle){   0,   0, 1200, 800 }, "Regular expressions");
                WindowActive = !GuiGroupBox((Rectangle){   0,    0, 1200, 800 }, NULL);
                if (GuiTextBox((Rectangle)             {  24,  16,   496,  24 }, options.InputPathText, 128, InputPathEditMode)) InputPathEditMode = !InputPathEditMode;
                if (GuiTextBox((Rectangle)             {  24,  56,   496,  24 }, options.FileTypeText, 128, FileTypeEditMode)) FileTypeEditMode = !FileTypeEditMode;
                if (GuiTextBox((Rectangle)             {  24,  96,   496,  24 }, OutputPathText, 128, OutputPathEditMode)) OutputPathEditMode = !OutputPathEditMode;
                if (GuiTextBox((Rectangle)             {  24, 136,   496,  24 }, RegularExpressionText, 128, RegularExpressionEditMode)) RegularExpressionEditMode = !RegularExpressionEditMode;
                GuiGroupBox((Rectangle)                { 544,   8,   304, 200 }, "Options");
                GuiCheckBox((Rectangle)                { 568,  16,    24,  24 }, "Print paths", &options.PrintPaths);
                GuiCheckBox((Rectangle)                { 568,  56,    24,  24 }, "Filter on filetype", &options.FilterTypes);
                GuiCheckBox((Rectangle)                { 568,  96,    24,  24 }, "AppendPaths", &options.AppendPaths);
                GuiCheckBox((Rectangle)                { 568, 136,    24,  24 }, "OmitMatches", &options.OmitMatches);
                ButtonSearch = GuiButton((Rectangle)   {  24, 176,   496,  24 }, "Search");
                TextSize = MeasureTextEx(JetBrainsMono, OutputText, GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING));
                GuiScrollPanel((Rectangle)             {  24, 216,   864, 350 },NULL, (Rectangle){ 80, 320, TextSize.x, TextSize.y }, &ScrollOutputScrollOffset, &ScrollOutputScrollView);

                BeginScissorMode(24, 216, 864, 350);
                {
                   DrawTextEx(JetBrainsMono, OutputText, (Vector2){ 24 + ScrollOutputScrollOffset.x, 216 + ScrollOutputScrollOffset.y },
                               GuiGetStyle(DEFAULT, TEXT_SIZE), 
                               GuiGetStyle(DEFAULT, TEXT_SPACING), 
                               GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                }
                EndScissorMode();

            //----------------------------------------------------------------------------------
            }
        EndDrawing();
        //----------------------------------------------------------------------------------
        if (ButtonSearch) {
            CompileCmd(cmd, options); 
            if (OpenProcess(&pipe, cmd)) return 1; reading = 1;
        }
    }

    // De-Initialization
    //--------------------------------------------------------------------------------------
    CloseWindow();        // Close window and OpenGL context
    //--------------------------------------------------------------------------------------

    return 0;
}

//------------------------------------------------------------------------------------
// Controls Functions Definitions (local)
//------------------------------------------------------------------------------------

