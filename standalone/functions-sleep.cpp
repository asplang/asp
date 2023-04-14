//
// Standalone Asp application functions implementation: sleep.
//

#include "asp.h"
#include "standalone.h"
#include "context.h"
#include <ctime>
#include <cmath>

/* sleep(s)
 * Sleep for s seconds.
 */
extern "C" AspRunResult asp_sleep
    (AspEngine *engine,
     AspDataEntry *sec,
     AspDataEntry **returnValue)
{
    auto context = reinterpret_cast<StandaloneAspContext *>
        (AspContext(engine));
    if (!AspAgain(engine))
    {
        double secValue;
        if (!AspFloatValue(sec, &secValue))
            return AspRunResult_UnexpectedType;
        context->expiry =
            clock() + static_cast<clock_t>(round(CLOCKS_PER_SEC * secValue));
    }

    return clock() < context->expiry ? AspRunResult_Again : AspRunResult_OK;
}
