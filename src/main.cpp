#include "app.hpp"
#include "config.hpp"
#include <iostream>
#include <csignal>
#include <cstring>

static whispr::App* g_app = nullptr;

void signal_handler(int signum) {
    std::cout << "\nReceived signal " << signum << ", shutting down..." << std::endl;
    if (g_app) {
        g_app->quit();
    }
}

void print_usage(const char* program) {
    std::cout << "Usage: " << program << " [options]\n"
              << "\nOptions:\n"
              << "  -m, --model PATH    Path to whisper model (default: models/ggml-base.en.bin)\n"
              << "  -t, --threads N     Number of CPU threads (default: 4)\n"
              << "  -l, --language LANG Language code (default: en)\n"
              << "  -k, --keycode N     Hotkey keycode (default: Right Option/Alt)\n"
              << "  --no-paste          Don't auto-paste, just copy to clipboard\n"
              << "  -h, --help          Show this help\n"
              << "\nHotkey:\n"
              << "  Hold the configured key to record, release to transcribe and paste.\n"
              << "  Default: Right Option (macOS) or Right Alt (Linux)\n"
              << "\nFirst run:\n"
              << "  1. Download a whisper model:\n"
              << "     curl -L -o models/ggml-base.en.bin \\\n"
              << "       https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin\n"
              << "  2. Grant accessibility permissions (macOS) or run with input group (Linux)\n"
              << std::endl;
}

int main(int argc, char* argv[]) {
    whispr::Config config;

    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--model") == 0) && i + 1 < argc) {
            config.model_path = argv[++i];
        }
        else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) && i + 1 < argc) {
            config.n_threads = std::atoi(argv[++i]);
        }
        else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--language") == 0) && i + 1 < argc) {
            config.language = argv[++i];
        }
        else if ((strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keycode") == 0) && i + 1 < argc) {
            config.hotkey_keycode = static_cast<uint32_t>(std::atoi(argv[++i]));
        }
        else if (strcmp(argv[i], "--no-paste") == 0) {
            config.auto_paste = false;
        }
        else {
            std::cerr << "Unknown option: " << argv[i] << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Create and initialize app
    whispr::App app;
    g_app = &app;

    std::cout << "Whispr Clone - Voice to Text\n" << std::endl;
    std::cout << "Model: " << config.model_path << std::endl;
    std::cout << "Threads: " << config.n_threads << std::endl;
    std::cout << "Language: " << config.language << std::endl;
    std::cout << "Auto-paste: " << (config.auto_paste ? "yes" : "no") << std::endl;
    std::cout << std::endl;

    if (!app.initialize(config)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }

    int result = app.run();

    g_app = nullptr;
    return result;
}
