/* NIGHT — SDL3 port of T.G Entertainment's "어쩐지 좋은일이 생길것 같은 저녁"
 * See docs/00-tasks.md. This file is the CLI entry point; self-test lives in
 * selftest.c, game modes (title/opening/stage1) are added by later tasks. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

int selftest(const char *res_path, const char *shot_dir);
void mkdir_shots(const char *dir);
int  game_run(int argc, char **argv);

int main(int argc, char **argv) {
    const char *res = "../NIGHT/NIGHT.RES";
    const char *shots = "../scratch/shots";
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--selftest")) {
            mkdir_shots(shots);
            return selftest(res, shots);
        }
        if (!strcmp(argv[i], "--res") && i + 1 < argc) res = argv[++i];
        if (!strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        if (!strcmp(argv[i], "--help")) {
            printf("usage: night [--selftest] [--res PATH] [--shots DIR]\n");
            return 0;
        }
    }
    return game_run(argc, argv);
}

/* mkdir -p for the shots dir (portable, shallow). */
void mkdir_shots(const char *dir) {
    char tmp[1024];
    snprintf(tmp, sizeof tmp, "%s", dir);
    for (char *p = tmp + 1; *p; p++) {
        if (*p == '/') { *p = 0; mkdir(tmp, 0755); *p = '/'; }
    }
    mkdir(tmp, 0755);
}
