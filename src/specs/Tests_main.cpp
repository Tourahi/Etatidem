#include "../lib/utest.h"
#include "../lib/aixlog.hpp"
#include "../config.h"

int main(int argc, char **argv)
{
    AixLog::Log::init(
    {
            std::make_shared<AixLog::SinkCout>(AixLog::Severity::trace, "%Y-%m-%d %H-%M-%S.#ms [#severity] (#tag) #message"),
        }
    );
#ifdef ENABLE_UTEST
    return utest_main(argc, argv);
#else
    return 1;
#endif
}

#ifdef ENABLE_UTEST
UTEST_STATE();
#endif