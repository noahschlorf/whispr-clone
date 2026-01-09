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
              << "  -q, --quality MODE  Quality mode: fast, balanced, accurate, best (default: balanced)\n"
              << "  -m, --model-dir DIR Directory containing models (default: models)\n"
              << "  -t, --threads N     Number of CPU threads (default: 4)\n"
              << "  -l, --language LANG Language code (default: en)\n"
              << "  -k, --keycode N     Hotkey keycode (default: Right Option/Alt)\n"
              << "  --no-paste          Don't auto-paste, just copy to clipboard\n"
              << "  --no-preprocess     Disable audio preprocessing\n"
              << "  -h, --help          Show this help\n"
              << "\nQuality Modes:\n"
              << "  fast     - Fastest, ~80% accuracy (tiny.en model)\n"
              << "  balanced - Good balance, ~85% accuracy (base.en model)\n"
              << "  accurate - High accuracy, ~92% accuracy (small.en model)\n"
              << "  best     - Highest accuracy, ~95% accuracy (medium.en model)\n"
              << "\nHotkey:\n"
              << "  Hold the configured key to record, release to transcribe and paste.\n"
              << "  Default: Right Option (macOS) or Right Alt (Linux)\n"
              << "\nFirst run:\n"
              << "  Download models with: ./scripts/download_models.sh\n"
              << "  Or manually: curl -L -o models/ggml-base.en.bin \\\n"
              << "    https://huggingface.co/ggerganov/whisper.cpp/resolve/main/ggml-base.en.bin\n"
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
        else if ((strcmp(argv[i], "-q") == 0 || strcmp(argv[i], "--quality") == 0) && i + 1 < argc) {
            const char* mode = argv[++i];
            if (strcmp(mode, "fast") == 0) {
                config.model_quality = whispr::ModelQuality::Fast;
            } else if (strcmp(mode, "balanced") == 0) {
                config.model_quality = whispr::ModelQuality::Balanced;
            } else if (strcmp(mode, "accurate") == 0) {
                config.model_quality = whispr::ModelQuality::Accurate;
            } else if (strcmp(mode, "best") == 0) {
                config.model_quality = whispr::ModelQuality::Best;
            } else {
                std::cerr << "Unknown quality mode: " << mode << std::endl;
                print_usage(argv[0]);
                return 1;
            }
        }
        else if ((strcmp(argv[i], "-m") == 0 || strcmp(argv[i], "--model-dir") == 0) && i + 1 < argc) {
            config.model_dir = argv[++i];
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
        else if (strcmp(argv[i], "--no-preprocess") == 0) {
            config.audio_preprocessing = false;
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

    std::cout << "VoxType - Voice to Text\n" << std::endl;
    std::cout << "Quality: " << whispr::get_profile(config.model_quality).name << std::endl;
    std::cout << "Model: " << config.get_model_path() << std::endl;
    std::cout << "Threads: " << config.n_threads << std::endl;
    std::cout << "Language: " << config.language << std::endl;
    std::cout << "Auto-paste: " << (config.auto_paste ? "yes" : "no") << std::endl;
    std::cout << "Audio preprocessing: " << (config.audio_preprocessing ? "yes" : "no") << std::endl;
    std::cout << std::endl;

    if (!app.initialize(config)) {
        std::cerr << "Failed to initialize application" << std::endl;
        return 1;
    }

    int result = app.run();

    g_app = nullptr;
    return result;
}
