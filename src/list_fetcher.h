#ifndef ECHMET_UPD_LIST_FETCHER_H
#define ECHMET_UPD_LIST_FETCHER_H

#include "echmetupdatecheck.h"

/*!
 * Downloaded list object
 */
struct DownloadedList {
	char *list;		/*!< Downloaded file as string */
	char *error_string;	/*!< CURL return code in case the retrieval failed */
};

/*!
 * Frees fetcher's internal resources
 */
void fetcher_cleanup(void);

/*!
 * Downloads list of updates from a given URL.
 *
 * @param[out] list Result of the operation.
 * @param[in] URL of the file to download.
 * @param[in] allow_insecure Allow HTTP and ignore TLS errors
 * @param[in] user_agent String to use as user agent. If <tt>NULL</tt>, no user agent
 *                       string is set.
 */
EUPDRetCode fetcher_fetch(struct DownloadedList *list, const char *url, const int allow_insecure,
			  const char *user_agent);

/*!
 * Initializes fetcher's internal resources
 */
void fetcher_init(void);

/*!
 * Frees downloaded list.
 *
 * @param[in] list List to free
 */
void fetcher_list_cleanup(struct DownloadedList *list);

#endif /* ECHMET_UPD_LIST_FETCHER_H */
