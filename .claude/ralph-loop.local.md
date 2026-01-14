---
active: true
iteration: 1
max_iterations: 100
completion_promise: "VOXTYPE_PRODUCTION_READY"
started_at: "2026-01-14T00:00:00Z"
---

Fix and improve VoxType following RALPH_WIGGUM_FIX_ALL.md.

Execute phases 1-10 in order. For each phase:
1. Implement the fixes described
2. Build and verify no new warnings
3. Run tests to ensure no regressions
4. Commit changes with descriptive message

**54 issues to fix across 10 phases:**

## Phase 1: Critical Bug Fixes (DO FIRST)
- Fix null pointer on std::getenv("HOME") in vocabulary.cpp:12
- Fix unsafe std::atoi() in main.cpp:71,77 - use std::stoi with try/catch
- Build and test

## Phase 2: Thread Safety & Memory
- Add mutex protection to global state in hotkey_macos.mm and tray_macos.mm
- Replace raw new/delete with std::unique_ptr in hotkey_macos.mm:27-39
- Fix memory leak in tray icons (add [release] calls in destroy_tray_icon)
- Synchronize g_history vector access with mutex

## Phase 3: Error Handling
- Add try/catch around audio_processor_->process() in app.cpp:197
- Improve whisper error messages with error codes in transcriber.cpp:115
- Add user notification for "no speech detected" in app.cpp:225

## Phase 4: Test Coverage
- Fix operator precedence bug in test_text_processor.cpp:122
- Create tests/test_integration.cpp with pipeline tests
- Create tests/test_edge_cases.cpp for empty/short/long audio

## Phase 5: UX Improvements
- Add model loading progress callback to transcriber.cpp
- Show confidence score in menu bar (tray_macos.mm)
- Add optional sound feedback config options

## Phase 6: Performance
- Pre-compile regex patterns at startup in text_processor.cpp
- Reuse buffers in audio_processor.cpp instead of allocating
- Add processing time metrics logging

## Phase 7: Security
- Validate user input paths in main.cpp (no path traversal)
- Sanitize vocabulary file content in vocabulary.cpp

## Phase 8: Documentation
- Add Linux installation section to README.md
- Create docs/CONFIGURATION.md with all options
- Add Doxygen comments to public methods

## Phase 9: Code Quality
- Add const correctness throughout
- Replace magic numbers with named constants in config.hpp
- Extract common platform code patterns

## Phase 10: Final Integration
- Full clean build (rm -rf build && ./build.sh)
- Run all tests (cd tests && ./run_tests.sh)
- Manual testing checklist
- Memory leak check with leaks or valgrind

Output <promise>VOXTYPE_PRODUCTION_READY</promise> when:
- All 10 phases complete
- All tests pass
- No memory leaks
- No compiler warnings
- Build succeeds on clean build

If blocked after 3 attempts on any item, document and skip to next.
