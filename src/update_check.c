#include "list_fetcher.h"
#include "list_parser.h"
#include "list_comparator.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define _STRINGIFY(input) #input
#define ERROR_CODE_CASE(erCase) case erCase: return _STRINGIFY(erCase)

/*!
 * Returns the actual or maximum length of software name
 *
 * @param[in] str String containing the name
 *
 * @return Length of the string
 */
static
size_t get_software_name_length(const char *str)
{
	const size_t stop = STRUCT_MEM_SZ(struct EUPDInSoftware, name);

	size_t idx;
	for (idx = 0; idx < stop; idx++) {
		if (str[idx] == '\0')
			break;
	}

	return idx;
}

/*!
 * \brief Checks if the content of \p InSoftware is valid
 *
 * Check whether the software name and revision contained
 * in \p InSoftware struct are sensible
 *
 * @param[in] sw Pointer to \p InSoftware struct to check
 *
 * @retval 0 Data is invalid
 * @retval 1 Data is valid
 */
static
int check_input(const struct EUPDInSoftware *sw)
{
	if (!is_software_name_valid(get_software_name_length(sw->name)))
		return 0;
	return is_revision_valid(sw->version.revision, STRUCT_MEM_SZ(struct EUPDVersion, revision));
}

/*!
 * \brief Builds user agent string
 *
 * Builds user agent string based on the ID of the software that
 * requests the update check. If no such information is pertitnent this value
 * may be <tt>NULL</tt>. In this case only the ID of the update check library
 * is returned in user agent string.
 *
 * @param[in] in_software ID of software requesting update check. This may be <tt>NULL</tt>
 *                        if no such information is available.
 *
 * @retval Pointer to user agent string on success
 * @retval <tt>NULL</tt> on failure
 */
static
char * make_user_agent_str(const struct EUPDInSoftware *in_software)
{
	static const char *PREFIX = "ECHMETUpdateCheck";
	const size_t MAX_SIZE = 64;

	char *user_agent = malloc(MAX_SIZE);
	if (user_agent == NULL)
		return NULL;

	memset(user_agent, 0, MAX_SIZE);

	if (in_software == NULL)
		snprintf(user_agent, MAX_SIZE, "%s", PREFIX);
	else {
		/* revision may not be be zero-terminated, make sure
		 * that the string we use here is */
		char rev_str[5];
		memcpy(rev_str, in_software->version.revision, 4);
		rev_str[4] = '\0';

		snprintf(user_agent, MAX_SIZE, "%s - %s %d.%d%s",
			 PREFIX,
			 in_software->name,
			 in_software->version.major,
			 in_software->version.minor,
			 rev_str);
	}

	return user_agent;
}

/*!
 * Downloads file containing list of updates fron a given URL
 *
 * @param[out] dl_list Initialized \p DownloadedList struct
 * @param[in] url URL of the file to download
 * @param[in] allow_insecure Allow HTTP and ignore TLS errors
 * @param[in] in_software ID of software requesting update check. This may be <tt>NULL</tt>
 *                        if no such information is available.
 *
 * @return EUPD_OK on success, appropriate error code oterwise
 */
static
EUPDRetCode fetch(struct DownloadedList *dl_list, const char *url, const int allow_insecure,
		  const struct EUPDInSoftware *in_software)
{
	EUPDRetCode tRet;

	char *user_agent = make_user_agent_str(in_software);

	fetcher_init();

	tRet = fetcher_fetch(dl_list, url, allow_insecure, user_agent);
	free(user_agent);

	fetcher_cleanup();

	return tRet;
}

/*!
 * Downloads list of updates form a given URL and parses the list
 *
 * @param[out] sw_list Initialized \p SoftwareList struct
 * @param[in] url URL of the file to download.
 * @param[in] allow_insecure Allow HTTP and ignore TLS errors
 * @param[in] in_software ID of software requesting update check. This may be <tt>NULL</tt>
 *                        if no such information is available.
 *
 * @retval EUPD_OK List successfully parsed
 * @retval EUPD_W_LIST_INCOMPLETE List contains invalid items and was not fully parsed
 * @return Appropriate error code if the list cannot be processed at all
 */
static
EUPDRetCode make_list(struct SoftwareList *sw_list, const char *url, const int allow_insecure,
		      const struct EUPDInSoftware *in_software)
{
	struct DownloadedList dl_list;
	EUPDRetCode tRet;

	tRet = fetch(&dl_list, url, allow_insecure, in_software);
	if (EUPD_IS_ERROR(tRet)) {
		fetcher_list_cleanup(&dl_list);

		return tRet;
	}

	tRet = parser_parse(dl_list.list, sw_list);
	fetcher_list_cleanup(&dl_list);

	return tRet;
}

/*!
 * Processes one item from list of updates
 *
 * @param[in] sw_list List of updates
 * @param[in] in_software Software whose update status is to be checked
 * @param[out] result Result of the update check
 * @param[in] in_ret Value of return code before this function was called.
 *                   This is necessary to retain any previous warning states.
 *
 * @return EUPD_OK or previous warning code on success, appropriate error code otherwise
 */
static
EUPDRetCode process_item(const struct SoftwareList *sw_list, const struct EUPDInSoftware *in_software,
			 struct EUPDResult *result, const EUPDRetCode in_ret)
{
	EUPDRetCode tRet;

	tRet = comparator_compare(sw_list, in_software, &result->status, &result->version);
	if (EUPD_IS_ERROR(tRet))
		return tRet;
	else if (EUPD_IS_WARNING(in_ret))
		tRet = in_ret;

	if (result->status != EUST_UNKNOWN) {
		EUPDRetCode tRetTwo = parser_set_link(sw_list, in_software->name, result);
		if (EUPD_IS_ERROR(tRetTwo))
			tRet = tRetTwo;
		else if (!EUPD_IS_WARNING(tRet))
			tRet = tRetTwo;
	}

	return tRet;
}

int is_revision_valid(const char *rev, const size_t len)
{
	size_t idx;

	if (len == 0)
		return 1;
	if (len > STRUCT_MEM_SZ(struct EUPDVersion, revision))
		return 0;

	if (!(isalpha(rev[0]) || rev[0] == '\0'))
		return 0;

	for (idx = 1; idx < len; idx++) {
		if (rev[idx] == '\0')
			return 1;
		else if (!(isalpha(rev[idx]) || isdigit(rev[idx])))
			return 0;
	}

	return 1;
}

int is_software_name_valid(const size_t len)
{
	return (len > 0 && len <= STRUCT_MEM_SZ(struct EUPDInSoftware, name));
}

EUPDRetCode ECHMET_CC updater_check(const char *url, const struct EUPDInSoftware *in_software,
				    struct EUPDResult *result, const int allow_insecure)
{
	struct SoftwareList sw_list;
	EUPDRetCode tRet;

	if (!check_input(in_software))
		return EUPD_E_INVALID_ARGUMENT;

	memset(&sw_list, 0, sizeof(struct SoftwareList));
	memset(result, 0, sizeof(struct EUPDResult));

	tRet = make_list(&sw_list, url, allow_insecure, in_software);
	if (EUPD_IS_ERROR(tRet))
		goto out;

	tRet = process_item(&sw_list, in_software, result, tRet);

out:
	parser_free_list(&sw_list);

	return tRet;
}

EUPDRetCode ECHMET_CC updater_check_many(const char *url, const struct EUPDInSoftware *in_software_list, const size_t num_software,
					 struct EUPDResult **out_results, size_t *num_results, const int allow_insecure)
{
	struct SoftwareList sw_list;
	EUPDRetCode tRet;
	struct EUPDResult *results;

	results = calloc(sizeof(struct EUPDResult), num_software);
	if (results == NULL)
		return EUPD_E_NO_MEMORY;

	memset(results, 0, sizeof(struct EUPDResult) * num_software);
	memset(&sw_list, 0, sizeof(struct SoftwareList));
	*num_results = 0;

	tRet = make_list(&sw_list, url, allow_insecure, NULL);
	if (EUPD_IS_ERROR(tRet))
		goto err_out;

	for (*num_results = 0; *num_results < num_software; (*num_results)++) {
		const struct EUPDInSoftware *in_sw = &in_software_list[*num_results];
		if (!check_input(in_sw)) {
			tRet = EUPD_E_INVALID_ARGUMENT;
			goto err_out;
		}

		tRet = process_item(&sw_list, in_sw,
				    &results[*num_results], tRet);
		if (EUPD_IS_ERROR(tRet))
			goto err_out;
	}

	parser_free_list(&sw_list);

	*out_results = results;

	return tRet;

err_out:
	parser_free_list(&sw_list);
	updater_free_result_list(results, *num_results);

	return tRet;
}

void ECHMET_CC updater_free_result(struct EUPDResult *result)
{
	free(result->link);
}

void ECHMET_CC updater_free_result_list(struct EUPDResult *results, const size_t num_results)
{
	size_t idx;
	for (idx = 0; idx < num_results; idx++)
		updater_free_result(&results[idx]);
	free(results);
}

const char * ECHMET_CC updater_error_to_str(const EUPDRetCode tRet)
{
	switch (tRet) {
		ERROR_CODE_CASE(EUPD_OK);
		ERROR_CODE_CASE(EUPD_W_LIST_INCOMPLETE);
		ERROR_CODE_CASE(EUPD_W_NOT_FOUND);
		ERROR_CODE_CASE(EUPD_E_NO_MEMORY);
		ERROR_CODE_CASE(EUPD_E_CURL_SETUP);
		ERROR_CODE_CASE(EUPD_E_CANNOT_RESOLVE);
		ERROR_CODE_CASE(EUPD_E_CONNECTION_FAILED);
		ERROR_CODE_CASE(EUPD_E_HTTP_ERROR);
		ERROR_CODE_CASE(EUPD_E_TRANSFER_ERROR);
		ERROR_CODE_CASE(EUPD_E_TIMEOUT);
		ERROR_CODE_CASE(EUPD_E_SSL);
		ERROR_CODE_CASE(EUPD_E_UNKW_NETWORK);
		ERROR_CODE_CASE(EUPD_E_MALFORMED_LIST);
		ERROR_CODE_CASE(EUPD_E_INVALID_ARGUMENT);
	default:
		return "Unknown error";
	}
}

const char * ECHMET_CC updater_status_to_str(const EUPDUpdateStatus status)
{
	switch (status) {
		ERROR_CODE_CASE(EUST_UNKNOWN);
		ERROR_CODE_CASE(EUST_UP_TO_DATE);
		ERROR_CODE_CASE(EUST_UPDATE_AVAILABLE);
		ERROR_CODE_CASE(EUST_UPDATE_RECOMMENDED);
		ERROR_CODE_CASE(EUST_UPDATE_REQUIRED);
	default:
		return "Invalid status";
	}
}
