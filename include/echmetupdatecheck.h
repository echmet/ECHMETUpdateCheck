#ifndef ECHMET_UPDATECHECK_H
#define ECHMET_UPDATECHECK_H

#include <echmetupdatecheck_config.h>
#include <stddef.h>

/* Enforce calling convention */
#ifndef ECHMET_CC
	#if defined ECHMET_PLATFORM_WIN32
		#define ECHMET_CC __cdecl
	#elif defined ECHMET_PLATFORM_UNIX
		#ifdef ECHMET_COMPILER_GCC_LIKE
			#ifdef __i386__
				#define ECHMET_CC __attribute__((__cdecl__))
			#else
				#define ECHMET_CC
			#endif /* __x86_64__ */
		#else
			#error "Unsupported or misdetected compiler"
		#endif /* ECHMET_COMPILER_* */
	#else
		#error "Unsupported or misdetected target platform"
	#endif /* ECHMET_PLATFORM_* */
#endif /* ECHMET_CC */

/* Allow for redefinitions of ECHMET_API as needed */
#ifdef ECHMET_API
	#undef ECHMET_API
#endif /* ECHMET_API */

/* Export only symbols that are part of the public API */
#if defined ECHMET_PLATFORM_WIN32
	#if defined ECHMET_DLL_BUILD && !defined(ECHMET_IMPORT_INTERNAL)
		#if defined ECHMET_COMPILER_MINGW || defined ECHMET_COMPILER_MSYS
			#define ECHMET_API __attribute__ ((dllexport))
		#elif defined ECHMET_COMPILER_MSVC
			#define ECHMET_API __declspec(dllexport)
		#else
			#error "Unsupported or misdetected compiler"
		#endif /* ECHMET_COMPILER_* */
	#else
		#if defined ECHMET_COMPILER_MINGW || defined ECHMET_COMPILER_MSYS
			#define ECHMET_API __attribute__ ((dllimport))
		#elif defined ECHMET_COMPILER_MSVC
			#define ECHMET_API __declspec(dllimport)
		#else
			#error "Unsupported or misdetected compiler"
		#endif /* ECHMET_COMPILER_* */
	#endif /* ECHMET_DLL_BUILD */
#elif defined ECHMET_PLATFORM_UNIX
	#ifdef ECHMET_COMPILER_GCC_LIKE
		#if defined ECHMET_DLL_BUILD && !defined(ECHMET_IMPORT_INTERNAL)
			#define ECHMET_API __attribute__ ((visibility ("default")))
		#else
			#define ECHMET_API
		#endif /* ECHMET_DLL_BUILD */
	#else
		#error "Unsupported or misdetected compiler"
	#endif /* ECHMET_COMPILER_* */
#else
	#error "Unsupported or misdetected target platform"
#endif /* ECHMET_PLATFORM_* */

/*! \def EUPD_IS_ERROR(err)
 * Macro that determines whether \p EUPDRetCode corresponds to an error state
 */
#define EUPD_IS_ERROR(err) \
	(err >= 0x200)

/*! \def EUPD_IS_NETWORK_ERROR(err)
 * Macro that determines whether \p EUPDRetCode corresponds to a network error state
 */
#define EUPD_IS_NETWORK_ERROR(err) \
	(err >= 0x300 && err < 0x400)

/*!
 * \def EUPD_IS_WARNING(err)
 * Macro that determines whether \p EUPDRetCode corresponds to a warning state
 */
#define EUPD_IS_WARNING(err) \
	(err >= 0x100 && err < 0x200)

#if __cplusplus
extern "C" {
#endif /* __cplusplus */

/*!
 * Library return codes
 */
typedef enum _EUPDRetCode {
	EUPD_OK,			/*!< Operation succeded */
	EUPD_W_LIST_INCOMPLETE = 0x100,	/*!< Downloaded list of updates was only partially processed */
	EUPD_W_NOT_FOUND,		/*!< Downloaded list does not contain requested software */
	EUPD_E_NO_MEMORY = 0x200,	/*!< Not enough memory to complete operation */
	EUPD_E_MALFORMED_LIST,		/*!< Downloaded list of updates is malformed and cannot be parsed */
	EUPD_E_INVALID_ARGUMENT,	/*!< Invalid argument was passed to function */
	EUPD_E_CURL_SETUP = 0x300,	/*!< Unable to set CURL parameters */
	EUPD_E_CANNOT_RESOLVE,		/*!< Cannot resolve remote host name */
	EUPD_E_CONNECTION_FAILED,	/*!< Failed to connect to remote host */
	EUPD_E_HTTP_ERROR,		/*!< HTTP error encoutered */
	EUPD_E_TRANSFER_ERROR,		/*!< Error occcured during data transfer */
	EUPD_E_TIMEOUT,			/*!< Connection timeout */
	EUPD_E_SSL,			/*!< SSL/TLS error */
	EUPD_E_UNKW_NETWORK,		/*!< Unspecified network error occured */
	EUPD_E__LAST = 0x400,
	EUPD__FORCE_INT32 = 0x7FFFFFF
} EUPDRetCode;

/*!
 * Software update status
 */
typedef enum _EUPDUpdateStatus {
	EUST_UNKNOWN,			/*!< Update status of software cannot be determined */
	EUST_UP_TO_DATE,		/*!< Software is up to date */
	EUST_UPDATE_AVAILABLE,		/*!< Update for software is available */
	EUST_UPDATE_RECOMMENDED,	/*!< Update for software is available and contains important fixes */
	EUST_UPDATE_REQUIRED,		/*!< Update for software is available and contains critical fixes */
	EUST__FORCE_INT32 = 0x7FFFFFF
} EUPDUpdateStatus;

/*!
 * Software version information
 */
struct EUPDVersion {
	int major;			/*!< Major version number */
	int minor;			/*!< Minor version number */
	char revision[4];		/*!< Revision tag.
					     If the revision is shorter than 4 characters it shall be zero-terminated */
};

/*!
 * Software descriptor
 */
struct EUPDInSoftware {
	char name[32];			/*!< Name of the software.
                                             If the name is shorter than 32 characters it shall be zero-terminated */
	struct EUPDVersion version;	/*!< Currently installed version of the software */
};

/*!
 * Result of update check.
 *
 * Memory claimed by the struct shall be free'd using \p updater_free_result() function
 */
struct EUPDResult {
	EUPDUpdateStatus status;	/*!< Update status */
	struct EUPDVersion version;	/*!< Latest available version */
	char *link;			/*!< Download link to the lastest version */
};

/*!
 * \brief Checks update status of one software.
 *
 * Checks if there is an update available for a given software.
 * If the function returns an error, contents of \p result are undefined and
 * the struct shall not be free'd.
 * If the update status in \p result is \p EUST_UNKNOWN, fields \p version
 * and \p link are undefined. Freeing the struct is possible but unnecessary.
 *
 * @param[in] url URL of updates list file
 * @param[in] in_software Descriptor of the software to check
 * @param[out] result Result of update check
 * @param[in] allow_insecure Allow HTTP and ignore TLS errors. This is dangerous and shall not be used
 *                           in production.
 *
 * @return \p EUPD_OK if check was performed successfully, appropriate warning or error otherwise
 */
ECHMET_API EUPDRetCode ECHMET_CC updater_check(const char *url, const struct EUPDInSoftware *in_software,
				               struct EUPDResult *result, const int allow_insecure);

/*!
 * \brief Checks update status of multiple softwares.
 *
 * Checks if there are updates available for a given list of softwares.
 * If the function returns an error, contents of \p results is undefined
 * and the memory shall not be free'd.
 * If the update status of any of the software is EUST_UNKNOWN, fields \p version
 * and \p link are undefined for that software.
 * If function at least partially succeeds, \p results shall be free'd using
 * \p updater_free_result_list().
 *
 * @param[in] url URL of updates list file
 * @param[in] in_software_list Array of descriptors of software to check
 * @param[in] num_software Length of the in_software_list array
 * @param[out] results Pointer to the array of results. The array will have the same ordering as
 *                     as \p in_software_list array. Value of \p results is defined only if
 *                     this function does not return an error.
 * @pararm[out] num_results Number of items if the \p results array. This number may be lower than
 *                          \p num_software in case the function succeeds only partially.
 * @param[in] allow_insecure Allow HTTP and ignore TLS errors. This is dangerous and shall not be used
 *                           in production.
 *
 * @return \p EUPD_OK if check was performed successfully, appropriate warning or error otherwise
 */
ECHMET_API EUPDRetCode ECHMET_CC updater_check_many(const char *url, const struct EUPDInSoftware *in_software_list,
						    const size_t num_software, struct EUPDResult **results, size_t *num_results,
						    const int allow_insecure);

/*!
 * Converts \p EUPDRetCode to string representation.
 *
 * @param[in] tRet Return code to convert
 *
 * @return String representation of the return code
 */
ECHMET_API const char * ECHMET_CC updater_error_to_str(const EUPDRetCode tRet);

/*!
 * Frees memory claimed by \p Result struct.
 *
 * @param[in] result Object to free
 */
ECHMET_API void ECHMET_CC updater_free_result(struct EUPDResult *result);

/*!
 * Frees memory claimed by array of \p Result s.
 *
 * @param[in] results Array to free.
 * @param[in] num_results Length of the array
 */
ECHMET_API void ECHMET_CC updater_free_result_list(struct EUPDResult *results, const size_t num_results);

/*!
 * Converts \p EUPDUpdateStatus to string representation.
 *
 * @param[in] status Status code to convert
 *
 * @return String representation of the return code
 */
ECHMET_API const char * ECHMET_CC updater_status_to_str(const EUPDUpdateStatus status);

#if __cplusplus
}
#endif /* __cplusplus */

#endif /* ECHMET_UPDATECHECK_H */
