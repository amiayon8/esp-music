#include "app_state.hpp"

extern "C" void app_main() {
    ApplicationController app;
    if (app.initialize()) {
        app.run();
    }
}
