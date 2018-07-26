#ifndef ECHMET_UPD_LIST_PARSER_H
#define ECHMET_UPD_LIST_PARSER_H

#include "echmetupdatecheck_p.h"

#include <echmetupdatecheck.h>
#include <stddef.h>

struct ListVersion {
	struct EUPDVersion version;
	Severity severity;
};

struct Software {
	char name[32];
	char *link;
	struct ListVersion *versions;
	size_t num_versions;
};

struct SoftwareList {
	struct Software *items;
	size_t length;
};

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * Frees parsed software list.
 *
 * @param[in] sw_list List to free
 */
void parser_free_list(struct SoftwareList *sw_list);

/*!
 * Parses downloaded software list.
 *
 * @param[in] list_string Software list as string.
 * @param[out] sw_list Parsed list
 *
 * @return EUPD_OK on success, appropriate warning if the list was only partially parsed
 *         or error if the list is completely unparsable.
 */
EUPDRetCode parser_parse(const char *list_string, struct SoftwareList *sw_list);

/*!
 * Assingns a download link to \p Results struct.
 *
 * @param[in] sw_list Parsed software list.
 * @param[in] name Name of the software.
 * @param[in,out] result \p Result for the software to assign the link to
 *
 * @retval EUPD_OK Success
 * @retval EUPD_E_NO_MEMORY Insufficient memory to complete operation
 */
EUPDRetCode parser_set_link(const struct SoftwareList *sw_list, const char *name, struct EUPDResult *result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* ECHMET_UPD_LIST_PARSER_H */
