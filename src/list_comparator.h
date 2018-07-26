#ifndef ECHMET_UPD_LIST_COMPARATOR_H
#define ECHMET_UPD_LIST_COMPARATOR_H

#include "list_parser.h"

#include <echmetupdatecheck.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * Walks through parsed list of available software and tries to determine if there
 * is an update available for the given software.
 *
 * @param[in] sw_list Parsed list of softwares
 * @param[in] checked_sw Software to check for update
 * @param[out] status Update status of the given software
 * @param[out] new_version Latest available version of the given software
 *
 * @retval EUPD_OK Check completed successfully
 * @retval EUPD_W_NOT_FOUND Given software was not found in the list
 */
EUPDRetCode comparator_compare(const struct SoftwareList *sw_list, const struct EUPDInSoftware *checked_sw,
			       EUPDUpdateStatus *status, struct EUPDVersion *new_version);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ECHMET_UPD_LIST_COMPARATOR_H */
