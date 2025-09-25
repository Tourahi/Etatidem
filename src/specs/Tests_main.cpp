#include "../lib/utest.h"
#include "../config.h"

int main(int argc, char **argv)
{
#ifdef ENABLE_UTEST
    return utest_main(argc, argv);
#else
    return 1;
#endif
}

#ifdef ENABLE_UTEST
UTEST_STATE();
#endif