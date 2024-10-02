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
    auto context = static_cast<StandaloneAspContext *>
        (AspContext(engine));

    clock_t ticks = clock();

    if (!AspAgain(engine))
    {
        double secValue;
        if (!AspFloatValue(sec, &secValue))
            return AspRunResult_UnexpectedType;
        context->expiry =
            ticks + static_cast<clock_t>(round(CLOCKS_PER_SEC * secValue));
        context->sleeping = true;
        return AspRunResult_Again;
    }

    return ticks < context->expiry ? AspRunResult_Again : AspRunResult_OK;
}
