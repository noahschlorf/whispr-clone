#include "app.hpp"
#include "config.hpp"
#include <iostream>
#include <csignal>
#include <cstring>
#include <stdexcept>
#include <filesystem>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#include <unistd.h>
#endif

// Get the resources directory if running from a macOS app bundle
// Returns empty string if not in a bundle or on other platforms
static std::string get_bundle_resources_path() {
#ifdef __APPLE__
    // Get the executable path
    char path[PATH_MAX];
    uint32_t size = sizeof(path);
    if (_NSGetExecutablePath(path, &size) == 0) {
        std::filesystem::path exe_path(path);
        exe_path = std::filesystem::canonical(exe_path);

        // Check if we're in a .app bundle: .../VoxType.app/Contents/MacOS/VoxType
        std::filesystem::path parent = exe_path.parent_path();  // MacOS
        if (parent.filename() == "MacOS") {
            std::filesystem::path contents = parent.parent_path();  // Contents
            if (contents.filename() == "Contents") {
                std::filesystem::path resources = contents / "Resources";
                if (std::filesystem::exists(resources)) {
                    return resources.string();
                }
            }
        }
    }
#endif
    return "";
}

// Change to bundle resources directory so Metal shader can be found
static void setup_bundle_environment(const std::string& resources_path) {
    if (!resources_path.empty()) {
        // Change to resources directory for Metal shader discovery
        if (chdir(resources_path.c_str()) == 0) {
            // Metal shader should now be findable in current directory
        }
    }
}

// Validate a user-provided path for security
static bool is_safe_path(const std::string& path) {
    // Check for path traversal attempts
    if (path.find("..") != std::string::npos) {
        std::cerr << "Error: Path traversal not allowed in path" << std::endl;
        return false;
    }

    // Check for null bytes (injection attempt)
    if (path.find('\0') != std::string::npos) {
        std::cerr << "Error: Invalid characters in path" << std::endl;
        return false;
    }

    // Resolve to canonical path and check it exists
    try {
        std::filesystem::path p(path);
        if (std::filesystem::exists(p)) {
            // Ensure it's a directory
            if (!std::filesystem::is_directory(p)) {
                std::cerr << "Error: Model path must be a directory" << std::endl;
                return false;
            }
        }
        // If doesn't exist, that's OK - we'll create or fail later
    } catch (const std::filesystem::filesystem_error& e) {
        std::cerr << "Error: Invalid path - " << e.what() << std::endl;
        return false;
    }

    return true;
}

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

    // Check if running from app bundle and set default model path
    std::string bundle_resources = get_bundle_resources_path();
    if (!bundle_resources.empty()) {
        // Change to resources directory so Metal shader can be found
        setup_bundle_environment(bundle_resources);

        std::filesystem::path models_path = std::filesystem::path(bundle_resources) / "models";
        if (std::filesystem::exists(models_path)) {
            config.model_dir = models_path.string();

            // Also check for accurate model and use it if available
            std::filesystem::path small_model = models_path / "ggml-small.en.bin";
            if (std::filesystem::exists(small_model)) {
                config.model_quality = whispr::ModelQuality::Accurate;
            }
        }
    }

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
            std::string model_dir = argv[++i];
            if (!is_safe_path(model_dir)) {
                print_usage(argv[0]);
                return 1;
            }
            config.model_dir = model_dir;
        }
        else if ((strcmp(argv[i], "-t") == 0 || strcmp(argv[i], "--threads") == 0) && i + 1 < argc) {
            try {
                config.n_threads = std::stoi(argv[++i]);
                if (config.n_threads <= 0) {
                    std::cerr << "Warning: Invalid thread count, using default (4)" << std::endl;
                    config.n_threads = 4;
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Invalid thread count '" << argv[i] << "', using default (4)" << std::endl;
                config.n_threads = 4;
            }
        }
        else if ((strcmp(argv[i], "-l") == 0 || strcmp(argv[i], "--language") == 0) && i + 1 < argc) {
            config.language = argv[++i];
        }
        else if ((strcmp(argv[i], "-k") == 0 || strcmp(argv[i], "--keycode") == 0) && i + 1 < argc) {
            try {
                int keycode = std::stoi(argv[++i]);
                if (keycode < 0) {
                    std::cerr << "Warning: Invalid keycode, using default" << std::endl;
                } else {
                    config.hotkey_keycode = static_cast<uint32_t>(keycode);
                }
            } catch (const std::exception& e) {
                std::cerr << "Warning: Invalid keycode '" << argv[i] << "', using default" << std::endl;
            }
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
