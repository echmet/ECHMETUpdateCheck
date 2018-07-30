#include "list_comparator.h"
#include "echmetupdatecheck_p.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define COMP_NUM(l, r) \
	if (l > r) return VER_NEWER; \
	else if (l < r) return VER_OLDER

#define LTR_TO_LWR(l, v) \
	if (isalpha(l)) v = tolower(l); \
	else v = l

typedef enum _VersionDiff {
	VER_OLDER,
	VER_SAME,
	VER_NEWER
} VersionDiff;


/*!
 * Compares two revisions.
 *
 * @param[in] first First revision to compare
 * @param[in] second Second revision to compare
 *
 * @retval VER_NEWER First revision is newer than second
 * @retval VER_OLDER First revision is older than second
 * @retval VER_SAME Both revisions are the same
 */
static
VersionDiff compare_revision(const char *first, const char *second)
{
	const size_t len = STRUCT_MEM_SZ(struct EUPDVersion, revision);
	size_t idx;

	for (idx = 0; idx < len; idx++) {
		int l, r;
		LTR_TO_LWR(first[idx], l);
		LTR_TO_LWR(second[idx], r);

		COMP_NUM(l, r);
	}

	return VER_SAME;
}

/*!
 * Deep-copies version struct
 *
 * @param[in] dst \p EUPDVersion struct copied to
 * @param[in] src \p EUPDVersion struct copied from
 */
static
void copy_version(struct EUPDVersion *dst, const struct EUPDVersion *src)
{
	dst->major = src->major;
	dst->minor = src->minor;
	memcpy(dst->revision, src->revision, STRUCT_MEM_SZ(struct EUPDVersion, revision));
}

/*!
 * Compares two versions.
 *
 * @param[in] first First version to compare
 * @param[in] second Second version to compare
 *
 * @retval VER_NEWER First version is newer than second
 * @retval VER_OLDER First version is older than second
 * @retval VER_SAME Both versions are the same
 */
static
VersionDiff compare_version(const struct EUPDVersion *first, const struct EUPDVersion *second)
{
	COMP_NUM(first->major, second->major);
	COMP_NUM(first->minor, second->minor);
	return compare_revision(first->revision, second->revision);
}

EUPDRetCode comparator_compare(const struct SoftwareList *sw_list, const struct EUPDInSoftware *checked_sw,
			       EUPDUpdateStatus *status, struct EUPDVersion *new_version)
{
	size_t idx;
	for (idx = 0; idx < sw_list->length; idx++) {
		const struct Software *sw = &sw_list->items[idx];

		if (!STRNICMP(sw->name, checked_sw->name, STRUCT_MEM_SZ(struct Software, name))) {
			size_t jdx;
			Severity severity = SEV_FEATURE;
			int update_available = 0;

			copy_version(new_version, &checked_sw->version);

			for (jdx = 0; jdx < sw->num_versions; jdx++) {
				const struct ListVersion *lv = &sw->versions[jdx];

				VersionDiff diff = compare_version(&lv->version, new_version);
				if (diff == VER_NEWER) {
					update_available = 1;
					copy_version(new_version, &lv->version);
				}

				diff = compare_version(&lv->version, &checked_sw->version);
				if (diff == VER_NEWER) {
					if (lv->severity > severity)
						severity = lv->severity;
				}
			}

			if (update_available) {
				switch (severity) {
				case SEV_FEATURE:
					*status = EUST_UPDATE_AVAILABLE;
					break;
				case SEV_BUGFIX:
					*status = EUST_UPDATE_RECOMMENDED;
					break;
				case SEV_CRITICAL:
					*status = EUST_UPDATE_REQUIRED;
					break;
				default:
					abort();
				}
			} else {
				*status = EUST_UP_TO_DATE;
			}
			return EUPD_OK;
		}
	}

	*status = EUST_UNKNOWN;
	return EUPD_W_NOT_FOUND;
}
