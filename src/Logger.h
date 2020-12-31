#include <imguial_term.h>
#include <Components.h>

class Logger: public lrcpp::Logger {
public:
    Logger() {}

    bool init();
    void destroy();
    void reset();
    void draw();

    virtual void vprintf(retro_log_level level, char const* format, va_list args) override;

protected:
    ImGuiAl::BufferedLog<65536> _logger;
};
