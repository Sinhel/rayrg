#include <SDL2/SDL.h>
#include <SDL2/SDL_video.h>
#include <string.h>
#include <limits.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_IMPLEMENTATION
#define NK_SDL_RENDERER_IMPLEMENTATION
#include "JetBrainsMono-Medium.h"
#include "nuklear.h"
#include "nuklear_sdl_renderer.h"
#include "regexsearcher.h"

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

static nk_bool nk_filter_reject_all(const struct nk_text_edit *box, nk_rune unicode){
    NK_UNUSED(box);
    NK_UNUSED(unicode);
    return nk_false;
}

static void nk_edit_string_zero_terminated_placeholder(struct nk_context *ctx, char *buffer, size_t buffer_size, const char *text){
    struct nk_rect bounds = nk_widget_bounds(ctx);

    nk_edit_string_zero_terminated(ctx, NK_EDIT_FIELD, buffer, buffer_size, nk_filter_default);

    if (buffer[0] == '\0') {
        struct nk_style_edit *style = &ctx->style.edit;
        struct nk_rect padded = nk_rect(
                bounds.x + style->padding.x, 
                bounds.y + style->padding.y, 
                bounds.w - style->padding.x * 2, 
                bounds.h - style->padding.y * 2
                );

        struct nk_color fg = style->text_normal;
        struct nk_color bg = nk_rgba(0,0,0,0);
        nk_draw_text(&ctx->current->buffer, padded, text, (int)strlen(text), ctx->style.font, bg, fg);
    }
}

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");

    SDL_Window *win = SDL_CreateWindow("RegexSearcher", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT,
                                       SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);
    if (!win) {
        SDL_Log("Error SDL_CreateWindow: %s", SDL_GetError());
        return -1;
    }
    SDL_SetWindowMinimumSize(win, 300, 300);

    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        SDL_Log("Error SDL_CreateRenderer: %s", SDL_GetError());
        SDL_DestroyWindow(win);
        SDL_Quit();
        return -1;
    }

    struct nk_context *ctx = nk_sdl_init(win, renderer);

    /* High-DPI scaling */
    int render_w, render_h, window_w, window_h;
    SDL_GetRendererOutputSize(renderer, &render_w, &render_h);
    SDL_GetWindowSize(win, &window_w, &window_h);
    float scale_x = (float)render_w / (float)window_w;
    float scale_y = (float)render_h / (float)window_h;
    SDL_RenderSetScale(renderer, scale_x, scale_y);

    /* Font setup */
    struct nk_font_atlas *atlas;
    struct nk_font_config config = nk_font_config(0);
    nk_sdl_font_stash_begin(&atlas);
    struct nk_font *font =
        nk_font_atlas_add_from_memory(atlas, JetBrainsMono_Medium_ttf, JetBrainsMono_Medium_ttf_len, 16.0f * scale_y, &config);
    nk_sdl_font_stash_end();
    if (font) font->handle.height /= scale_y;
    nk_style_set_font(ctx, &font->handle);

    bool ButtonSearch = false;

    Options options = {0};
    FILE *pipe = {0};
    int reading = 0;
    char line[MAX_LINESIZE] = {0};
    char cmd[MAX_LINESIZE] = {0};
    CSV csv = {0};
    char *OutputText = malloc(1024 * 8);
    strcpy(OutputText, "Output");

    int running = 1;
    while (running) {
        //Handle closing of window
        SDL_Event evt;
        nk_input_begin(ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_QUIT) running = 0;
            else if (evt.type == SDL_KEYDOWN && evt.key.keysym.sym == SDLK_q && (SDL_GetModState() & KMOD_CTRL)) running = 0;
            else nk_sdl_handle_event(&evt);
        }
        nk_input_end(ctx);

        if (reading) {
            // Do 1000 reads per frame, to not slow down reading too much
            for (int i = 0; i < 1000; i++) {
                if (fgets(line, MAX_LINESIZE, pipe) != NULL) {
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

        //Get window size in case it's been resized
        static int win_w, win_h;
        SDL_GetWindowSize(win, &win_w, &win_h);

        if (nk_begin(ctx, "Regex Searcher", nk_rect(0, 0, win_w, win_h), NK_WINDOW_NO_SCROLLBAR)) {
            nk_layout_row_begin(ctx, NK_DYNAMIC, win_h/3.f, 2);

            nk_layout_row_push(ctx, 2.f/3.f);
            if (nk_group_begin(ctx, "Text fields", NK_WINDOW_TITLE | NK_WINDOW_BORDER)) {
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_edit_string_zero_terminated_placeholder(ctx, options.InputPathText, sizeof(options.InputPathText), "Input Path");
                nk_edit_string_zero_terminated_placeholder(ctx, options.FileText, sizeof(options.FileText), "File filter");
                nk_edit_string_zero_terminated_placeholder(ctx, options.OutputPathText, sizeof(options.OutputPathText), "Output Path");
                nk_edit_string_zero_terminated_placeholder(ctx, options.RegularExpressionText,sizeof(options.RegularExpressionText), "Regular Expression");
                nk_edit_string_zero_terminated_placeholder(ctx, options.DelimiterText, sizeof(options.DelimiterText), "Delimeter");
                if (nk_button_label(ctx, "Search")) {ButtonSearch = true;};
                nk_group_end(ctx);
            } 
            nk_layout_row_push(ctx, 1.0/3.0);
            if (nk_group_begin(ctx, "Options", NK_WINDOW_TITLE | NK_WINDOW_BORDER)) {
                nk_layout_row_dynamic(ctx, 30, 1);
                nk_checkbox_label(ctx, "Print paths",   &options.PrintPaths);
                nk_checkbox_label(ctx, "Append paths",  &options.AppendPaths);
                nk_checkbox_label(ctx, "Omit Matches",  &options.OmitMatches);
                nk_checkbox_label(ctx, "Multiline",     &options.Multiline);
                nk_checkbox_label(ctx, "Format",        &options.Format);
                nk_checkbox_label(ctx, "Debug command", &options.Debug);
                nk_group_end(ctx);
            }
            nk_layout_row_end(ctx);

            float output_height = win_h/3.f*2;

            float line_height = ctx->style.font->height
                              + 2*ctx->style.window.padding.y
                              + ctx->style.window.spacing.y;

            float row_height = ctx->style.font->height
                              + 2*ctx->style.window.spacing.y
                              + 2*ctx->style.window.padding.y
                              + 2*ctx->style.window.group_padding.y;
            
            float debug_rows = 2;
            if (options.Debug) output_height = output_height - (debug_rows*row_height);

            nk_layout_row_begin(ctx, NK_DYNAMIC, output_height, 1);
            nk_layout_row_push(ctx, 1.0);
            nk_edit_string_zero_terminated(ctx, NK_EDIT_SELECTABLE | NK_EDIT_BOX, OutputText, INT_MAX, nk_filter_reject_all);
            nk_layout_row_end(ctx);

            if (options.Debug) {
                strcpy(cmd, "");
                CompileCmd(cmd, options, &csv);

                nk_layout_row_begin(ctx, NK_DYNAMIC, line_height, 1);
                nk_layout_row_push(ctx, 1.0);
                nk_edit_string_zero_terminated_placeholder(ctx, options.DebugOptions ,sizeof(options.DebugOptions), "Debug options");
                nk_layout_row_end(ctx);

                nk_layout_row_begin(ctx, NK_DYNAMIC, line_height, 1);
                nk_layout_row_push(ctx, 1.0);
                nk_edit_string_zero_terminated_placeholder(ctx, cmd, sizeof(cmd), "Cmd");
                nk_layout_row_end(ctx);
            }
        }
        nk_end(ctx);

        nk_sdl_render(NK_ANTI_ALIASING_ON);
        SDL_RenderPresent(renderer);

        if (ButtonSearch || nk_input_is_key_pressed(&ctx->input, NK_KEY_ENTER)) {
            ButtonSearch = 0;
            
            WriteBuffer(NULL, &OutputText, true);
            if (options.Format) {WriteFormatHeader(options, &csv, &OutputText);}
            strcpy(cmd, "");
            if (CompileCmd(cmd, options, &csv)) {
                continue;
            }
            if (!OpenProcess(&pipe, cmd)) {
                reading = 1;
            }
        }
    }

    free(OutputText);

    nk_sdl_shutdown();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
