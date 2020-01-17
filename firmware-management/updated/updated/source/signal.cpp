#include "signal.h"

namespace updated {
namespace signal {

extern "C" void sigint_handler(int sig)
{
    sigint = sig;
}

extern "C" void sighup_handler(int sig)
{
    sighup = sig;
}

void register_handlers()
{
    std::signal(SIGINT, sigint_handler);
    std::signal(SIGHUP, sighup_handler);
}


} // namespace signal
} // namespace updated
