/* SPDX-License-Identifier: MPL-2.0 */
#include <maelys/datalog.h>
#include <stdio.h>
int main(void) {
    size_t facts = 0;
#ifdef MAELYS_DATALOG_PROFILE_LARGE
    const size_t expected = 256;
#else
    const size_t expected = 64;
#endif
    if (maelys_datalog_limit_get(MAELYS_DATALOG_LIMIT_MAX_FACTS_PER_PRED, &facts)
            != MAELYS_DATALOG_STATUS_OK || facts != expected) {
        fprintf(stderr, "Installed SDK profile mismatch: expected %zu facts per predicate, got %zu\n", expected, facts);
        return 1;
    }
    return 0;
}
