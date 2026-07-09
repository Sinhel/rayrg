#include "raylib.h"
#define RAYGUI_IMPLEMENTATION
#include "raygui.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "JetBrainsMono-Medium.h"
#include "regexsearcher.h"

#define LINESIZE 2048

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
    while (!WindowShouldClose())
    {
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
            if (GuiTextBox((Rectangle){24, 56, 496, 24}, options.FileText, 128, FileEditMode)) FileEditMode = !FileEditMode;
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
        // Tie formatting option to always omit full matches
        if (options.Format) options.OmitMatches = true;

        if (ButtonSearch || IsKeyPressed(KEY_ENTER)) {
            WriteBuffer(NULL, &OutputText, true);
            strcpy(cmd, "");
            if (CompileCmd(cmd, options, &OutputText)) {
                // Handle compilation error
                continue;
            }
            if (!OpenProcess(&pipe, cmd)) {
                reading = 1;
            }
        }
    }
    CloseWindow();
    return 0;
}

