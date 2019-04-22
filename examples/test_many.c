#include <echmetupdatecheck.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main()
{
	const struct EUPDInSoftware inSwList[] =
	{
		{
			"Doomsday machine",
			{
				1,
				1,
				"c"
			}
		},
		{
			"Armageddon architect",
			{
				0,
				1,
				""
			}
		},
		{
			"Vortex of void",
			{
				12,
				5,
				"a"
			}
		},
		{
			"Microsoft Windows",
			{
				10,
				1701,
				""
			}
		}
	};

	const size_t sw_len = sizeof(inSwList) / sizeof(inSwList[0]);
	struct EUPDResult *results;
	size_t num_results;
	size_t idx;
	EUPDRetCode ret;

	printf("List of size %zu\n", sw_len);

	ret = updater_check_many("http://devoid-pointer.net/misc/curl_test.txt",
					     inSwList, sw_len,
					     &results, &num_results,
					     0);

	printf("Check result: %d (%s)\n", ret, updater_error_to_str(ret));

	if (EUPD_IS_ERROR(ret))
		return 1;

	for (idx = 0; idx < num_results; idx++) {
		const struct EUPDResult *r = &results[idx];
		const size_t len = sizeof(r->version.revision) + 1;
		char rev[len];

		if (r->status == EUST_UNKNOWN) {
			printf("Update status of \"%s\" could not have been checked\n", inSwList[idx].name);
			continue;
		}

		printf("%s\n", inSwList[idx].name);
		printf("Update: %s\n", updater_status_to_str(r->status));

		memcpy(rev, r->version.revision, len - 1);
		rev[len - 1] = '\0';
		printf("Latest version: %d.%d%s\nLink: %s\n", r->version.major,
							      r->version.minor, rev,
							      r->link);
	}

	updater_free_result_list(results, num_results);

	return 0;
}
