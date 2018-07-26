#ifndef ECHMET_UPDATECHECK_P_H
#define ECHMET_UPDATECHECK_P_H

#include <stddef.h>

#define STRUCT_MEM_SZ(t, m) (sizeof(((t *)0)->m))

typedef enum _Severity {
	SEV_FEATURE = 0,
	SEV_BUGFIX = 1,
	SEV_CRITICAL = 2
} Severity;

#ifdef __cplusplus
extern "C" {
#endif /* __clusplus */

/*!
 * Checks whether revision string is valid
 *
 * @param[in] rev Revision string
 * @param[in] len Maximum length of the revision string
 *
 * @retval 1 Revision string is valid
 * @retval 0 Revision string is invalid
 */
int is_revision_valid(const char *rev, const size_t len);

/*!
 * Checks whether software name string is valid
 *
 * @param[in] Length of the name string
 *
 * @retval 1 Name string is valid
 * @retval 0 Name string is invalid
 */
int is_software_name_valid(const size_t len);

#ifdef __cplusplus
}
#endif /* __clusplus */

#endif /* ECHMET_UPDATECHECK_P_H */
