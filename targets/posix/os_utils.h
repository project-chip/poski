
#ifndef _OS_UTILS_H
#define _OS_UTILS_H

#include <poski/osal/osal.h>
#include <assert.h>

#define SuccessOrQuit(ERR, MSG) \
  assert((MSG, ERR == 0))

#define VerifyOrQuit(TST, MSG) \
  assert((MSG, TST))

/**
 * This macro checks for the specified status, which is expected to commonly be
 * successful, and branches to the local label 'exit' if the status is
 * unsuccessful.
 *
 * @param[in]  aStatus     A scalar status to be evaluated against zero (0).
 *
 */
#define SuccessOrExit(aStatus) \
  do {                         \
    if ((aStatus) != 0) {      \
      goto exit;               \
    }                          \
  } while (false)

/**
 * This macro checks for the specified condition, which is expected to commonly
 * be true, and both executes @a ... and branches to the local label 'exit' if
 * the condition is false.
 *
 * @param[in]  aCondition  A Boolean expression to be evaluated.
 * @param[in]  aAction     An expression or block to execute when the assertion
 * fails.
 *
 */
#define VerifyOrExit(aCondition, aAction) \
  do {                                    \
    if (!(aCondition)) {                  \
      aAction;                            \
      goto exit;                          \
    }                                     \
  } while (false)

chip_os_error_t map_posix_to_osal_error(int ret);

#endif // _OS_UTILS_H
