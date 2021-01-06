#include "Application.h"

#include <stdlib.h>
#include <sys/stat.h>

int main(int, char**) {
    hc::Application app;

    if (!app.init("Hackable Console", 1024, 640)) {
        return EXIT_FAILURE;
    }

    app.run();
    app.destroy();
    return EXIT_SUCCESS;
}
