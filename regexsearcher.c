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
#include "JetBrainsMono-Medium.h"

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
    bool AppendPaths;
    bool OmitMatches;
    bool Multiline;
    char InputPathText[1024];
    char FileText[128];
    char OutputPathText[1024];
    char RegularExpressionText[2048];
} Options;

//----------------------------------------------------------------------------------
// Controls Functions Declaration
//----------------------------------------------------------------------------------


void CompileCmd(char *cmd, Options options) {
    char buffer[4096];

    strcat(cmd, "rg ");
    if (options.PrintPaths)  {strcat(cmd, "--files ");}
    if (options.Multiline)   {strcat(cmd, "-U ");}
    
    if (strcmp(options.FileText, "")){
        sprintf(buffer, "-g '%s' ", options.FileText); 
        strcat(cmd, buffer); 
    }

    if (!options.AppendPaths) {strcat(cmd, "-I ");}
    if (options.OmitMatches) {strcat(cmd, "--only-matching ");}

    if (strcmp(options.RegularExpressionText, "")){
        sprintf(buffer, "'%s' ", options.RegularExpressionText); 
        strcat(cmd, buffer);
    }

    if (strcmp(options.InputPathText, "")){
        sprintf(buffer, "%s ", options.InputPathText); 
        strcat(cmd, options.InputPathText);
    }

    //make sure stderr is included in stdout
    strcat(cmd, " 2>&1");

    //TODO remove
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

    Font JetBrainsMono = LoadFontFromMemory(".ttf", JetBrainsMono_Medium_ttf, JetBrainsMono_Medium_ttf_len, 32, NULL, 0);
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

    char *OutputText = malloc(1024*8);
    strcpy(OutputText, "Output");

    Vector2 TextSize = {0};

    Rectangle ScrollOutputScrollView = {0, 0, 0, 0};
    Vector2 ScrollOutputScrollOffset =  {0, 0};

    FILE *pipe = {0};
    int reading = 0;
    size_t total_size = 0;
    char line[LINESIZE] = {0};
    char cmd[LINESIZE] = {0};
    Options options = {0};
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
                //Make 1000 reads per frame, to not slow down reading to much (possible to do 60.000 lines per second with this loop)
                for (int i = 0; i < 1000; i++) {
                    if (!ReadProcess(pipe, line, &OutputText, &total_size)) reading = 1;
                    else { 
                        reading = 0;

                        int status = PCLOSE(pipe);
                        if (status == -1) {
                            perror("pclose failed");
                            return 1;
                        }
                        //cleanup and leave loop
                        if (strcmp(options.OutputPathText, "")) {WriteToFile(OutputText, options.OutputPathText);}
                        strcpy(cmd, "");
                        break;
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
                if (GuiTextBox((Rectangle)             {  24,  56,   496,  24 }, options.FileText, 128, FileEditMode)) FileEditMode = !FileEditMode;
                if (GuiTextBox((Rectangle)             {  24,  96,   496,  24 }, options.OutputPathText, 128, OutputPathEditMode)) OutputPathEditMode = !OutputPathEditMode;
                if (GuiTextBox((Rectangle)             {  24, 136,   496,  24 }, options.RegularExpressionText, 128, RegularExpressionEditMode)) RegularExpressionEditMode = !RegularExpressionEditMode;
                GuiGroupBox((Rectangle)                { 544,   8,   304, 200 }, "Options");
                GuiCheckBox((Rectangle)                { 568,  16,    24,  24 }, "Print paths", &options.PrintPaths);
                GuiCheckBox((Rectangle)                { 568,  56,    24,  24 }, "AppendPaths", &options.AppendPaths);
                GuiCheckBox((Rectangle)                { 568,  96,    24,  24 }, "OmitMatches", &options.OmitMatches);
                GuiCheckBox((Rectangle)                { 568, 136,    24,  24 }, "Multiline",   &options.Multiline);
                ButtonSearch = GuiButton((Rectangle)   {  24, 176,   496,  24 }, "Search");
                TextSize = MeasureTextEx(JetBrainsMono, OutputText, GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING));
                GuiScrollPanel((Rectangle)             {  24, 216,   864, 350 },NULL, (Rectangle){ 80, 320, TextSize.x, TextSize.y }, &ScrollOutputScrollOffset, &ScrollOutputScrollView);

                //Draw default values in textboxes
                {
                    if (!strcmp(options.InputPathText, "")) {
                        DrawTextEx(JetBrainsMono, "InputPath", (Vector2){ 28, 19 }, 
                            GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                    }
                    if (!strcmp(options.FileText, "")) {
                        DrawTextEx(JetBrainsMono, "File", (Vector2){ 28, 59 }, 
                            GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                    }
                    if (!strcmp(options.OutputPathText, "")) {
                        DrawTextEx(JetBrainsMono, "OutputPath", (Vector2){ 28, 99 }, 
                            GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                    }
                    if (!strcmp(options.RegularExpressionText, "")) {
                        DrawTextEx(JetBrainsMono, "RegularExpression", (Vector2){ 28, 139 }, 
                            GuiGetStyle(DEFAULT, TEXT_SIZE), GuiGetStyle(DEFAULT, TEXT_SPACING), GetColor(GuiGetStyle(DEFAULT, TEXT_COLOR_NORMAL)));
                    }
                }
                //Draw content in GuiScrollPanel
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
        if (ButtonSearch || IsKeyPressed(KEY_ENTER)) {
            CompileCmd(cmd, options); 
            if (!OpenProcess(&pipe, cmd)) { reading = 1; }
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

