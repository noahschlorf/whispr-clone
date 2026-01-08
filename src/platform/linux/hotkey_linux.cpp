#include "hotkey_manager.hpp"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/input.h>
#include <sys/select.h>

#ifdef HAS_LIBEVDEV
#include <libevdev/libevdev.h>
#endif

namespace whispr {

struct LinuxHotkeyState {
    int keyboard_fd = -1;
#ifdef HAS_LIBEVDEV
    struct libevdev* dev = nullptr;
#endif
    HotkeyManager* manager = nullptr;
    uint32_t target_keycode = 0;
    bool key_pressed = false;
};

static LinuxHotkeyState* g_state = nullptr;

bool HotkeyManager::initialize() {
    if (platform_handle_) return true;

    g_state = new LinuxHotkeyState();
    g_state->manager = this;
    platform_handle_ = g_state;

    return true;
}

void HotkeyManager::shutdown() {
    stop();

    if (g_state) {
#ifdef HAS_LIBEVDEV
        if (g_state->dev) {
            libevdev_free(g_state->dev);
        }
#endif
        if (g_state->keyboard_fd >= 0) {
            close(g_state->keyboard_fd);
        }
        delete g_state;
        g_state = nullptr;
    }
    platform_handle_ = nullptr;
}

bool HotkeyManager::start() {
    if (running_.load()) return true;
    if (!g_state) return false;

    g_state->target_keycode = keycode_;
    g_state->key_pressed = false;

    // Try to find keyboard device
    const char* keyboard_paths[] = {
        "/dev/input/event0",
        "/dev/input/event1",
        "/dev/input/event2",
        "/dev/input/by-path/platform-i8042-serio-0-event-kbd",
        nullptr
    };

    for (int i = 0; keyboard_paths[i]; ++i) {
        int fd = open(keyboard_paths[i], O_RDONLY | O_NONBLOCK);
        if (fd >= 0) {
#ifdef HAS_LIBEVDEV
            int rc = libevdev_new_from_fd(fd, &g_state->dev);
            if (rc >= 0) {
                // Check if it's a keyboard
                if (libevdev_has_event_type(g_state->dev, EV_KEY) &&
                    libevdev_has_event_code(g_state->dev, EV_KEY, KEY_A)) {
                    g_state->keyboard_fd = fd;
                    std::cout << "Using keyboard: " << keyboard_paths[i] << std::endl;
                    break;
                }
                libevdev_free(g_state->dev);
                g_state->dev = nullptr;
            }
#else
            g_state->keyboard_fd = fd;
            std::cout << "Using keyboard: " << keyboard_paths[i] << std::endl;
            break;
#endif
            close(fd);
        }
    }

    if (g_state->keyboard_fd < 0) {
        std::cerr << "Failed to open keyboard device. Try running with sudo or add user to input group." << std::endl;
        return false;
    }

    running_.store(true);

    // Start listener thread
    listener_thread_ = std::thread([this]() {
        run_loop();
    });

    return true;
}

void HotkeyManager::stop() {
    if (!running_.load()) return;

    running_.store(false);

    if (listener_thread_.joinable()) {
        listener_thread_.join();
    }
}

void HotkeyManager::run_loop() {
    struct input_event ev;

    while (running_.load()) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(g_state->keyboard_fd, &fds);

        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms timeout

        int ret = select(g_state->keyboard_fd + 1, &fds, nullptr, nullptr, &tv);
        if (ret <= 0) continue;

#ifdef HAS_LIBEVDEV
        int rc;
        do {
            rc = libevdev_next_event(g_state->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
            if (rc == 0 && ev.type == EV_KEY && ev.code == g_state->target_keycode) {
                bool pressed = (ev.value == 1);
                bool released = (ev.value == 0);

                if (pressed && !g_state->key_pressed) {
                    g_state->key_pressed = true;
                    if (callback_) callback_(true);
                } else if (released && g_state->key_pressed) {
                    g_state->key_pressed = false;
                    if (callback_) callback_(false);
                }
            }
        } while (rc == 0 || rc == 1);
#else
        ssize_t n = read(g_state->keyboard_fd, &ev, sizeof(ev));
        if (n == sizeof(ev) && ev.type == EV_KEY && ev.code == g_state->target_keycode) {
            bool pressed = (ev.value == 1);
            bool released = (ev.value == 0);

            if (pressed && !g_state->key_pressed) {
                g_state->key_pressed = true;
                if (callback_) callback_(true);
            } else if (released && g_state->key_pressed) {
                g_state->key_pressed = false;
                if (callback_) callback_(false);
            }
        }
#endif
    }
}

} // namespace whispr
