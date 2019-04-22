#include <echmetupdatecheck.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main()
{
	struct EUPDInSoftware inSw = {
		"Doomsday machine",
		1,
		1,
		"c"
	};
	struct EUPDResult result;

	EUPDRetCode ret = updater_check("http://devoid-pointer.net/misc/curl_test.txt",
					&inSw, &result, 0);

	printf("Result: %s, Update: %s\n", updater_error_to_str(ret),
					   updater_status_to_str(result.status));

	if (result.status == EUST_UNKNOWN)
		return 1;

	const size_t len = sizeof(result.version.revision) + 1;
	char rev[len];
	memcpy(rev, result.version.revision, len - 1);
	rev[len - 1] = '\0';
	printf("Latest version: %d.%d%s\nLink: %s\n", result.version.major,
						      result.version.minor, rev,
						      result.link);

	updater_free_result(&result);

	return 0;
}
