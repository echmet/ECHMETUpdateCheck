#include "list_fetcher.h"

#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>

struct Buffer {
	char *data;
	size_t length;
	size_t allocated;
};

struct Session {
	CURL *connection;
	char *error_string;
	struct Buffer data_buffer;
};

static
int writer(char *data, size_t size, size_t nmemb, void *raw)
{
	struct Buffer *buf = (struct Buffer *)raw;
	const size_t payload_size = size * nmemb;

	if (!buf)
		return 0;

	if (buf->length + payload_size > buf->allocated) {
		const size_t size_new = buf->allocated + payload_size;
		char *data_new = (char *)realloc(buf->data, size_new);
		if (!data_new)
			return 0;
		buf->data = data_new;
		buf->allocated = size_new;
	}

	memcpy(buf->data + buf->length, data, payload_size);
	buf->length += payload_size;

	return payload_size;
}

static
EUPDRetCode init_session(struct Session *s)
{
	EUPDRetCode ret;
	CURLcode curl_ret;

	memset(s, 0, sizeof(struct Session));

	s->error_string = (char *)malloc(CURL_ERROR_SIZE);
	if (!s->error_string) {
		ret = EUPD_E_NO_MEMORY;
		goto err_out;
	}
	memset(s->error_string, 0, CURL_ERROR_SIZE);

	s->data_buffer.data = NULL;
	s->data_buffer.allocated = 0;

	s->connection = curl_easy_init();

	if (!s->connection) {
		ret = EUPD_E_NO_MEMORY;
		goto err_out_2;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_ERRORBUFFER, s->error_string);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_WRITEFUNCTION, writer);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_WRITEDATA, &s->data_buffer);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_FOLLOWLOCATION, 1L);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_FAILONERROR, 1L);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_CONNECTTIMEOUT, 10L);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	curl_ret = curl_easy_setopt(s->connection, CURLOPT_TIMEOUT, 15L);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out_3;
	}

	return EUPD_OK;

err_out_3:
	curl_easy_cleanup(s->connection);
err_out_2:
	free(s->data_buffer.data);
err_out:
	free(s->error_string);

	return ret;
}

static
void destroy_session(struct Session *s)
{
	curl_easy_cleanup(s->connection);

	free(s->data_buffer.data);
	free(s->error_string);
}

void fetcher_cleanup(void)
{
	curl_global_cleanup();
}

EUPDRetCode fetcher_fetch(struct DownloadedList *list, const char *url, const int allow_insecure,
			  const char *user_agent)
{
	EUPDRetCode ret;
	CURLcode curl_ret;
	struct Session s;
	size_t len;

	memset(list, 0, sizeof(struct DownloadedList));

	ret = init_session(&s);
	if (ret != EUPD_OK)
		return ret;

	curl_ret = curl_easy_setopt(s.connection, CURLOPT_URL, url);
	if (curl_ret != CURLE_OK) {
		ret = EUPD_E_CURL_SETUP;
		goto err_out;
	}

	if (allow_insecure > 0) {
		curl_ret = curl_easy_setopt(s.connection, CURLOPT_SSL_VERIFYPEER, 0L);
		if (curl_ret != CURLE_OK) {
			ret = EUPD_E_CURL_SETUP;
			goto err_out;
		}

		curl_ret = curl_easy_setopt(s.connection, CURLOPT_SSL_VERIFYHOST, 0L);
		if (curl_ret != CURLE_OK) {
			ret = EUPD_E_CURL_SETUP;
			goto err_out;
		}
	}

	if (user_agent != NULL)
		curl_easy_setopt(s.connection, CURLOPT_USERAGENT, user_agent);

	curl_ret = curl_easy_perform(s.connection);
	switch (curl_ret) {
	case CURLE_OK:
		break;
	case CURLE_COULDNT_RESOLVE_HOST:
		ret = EUPD_E_CANNOT_RESOLVE;
		goto err_out_2;
	case CURLE_COULDNT_CONNECT:
		ret = EUPD_E_CONNECTION_FAILED;
		goto err_out_2;
	case CURLE_HTTP_RETURNED_ERROR:
		ret = EUPD_E_HTTP_ERROR;
		goto err_out_2;
	case CURLE_WRITE_ERROR:
		ret = EUPD_E_TRANSFER_ERROR;
		goto err_out_2;
	case CURLE_OPERATION_TIMEDOUT:
		ret = EUPD_E_TIMEOUT;
		goto err_out_2;
	case CURLE_SSL_CONNECT_ERROR:
		ret = EUPD_E_SSL;
		goto err_out_2;
	default:
		ret = EUPD_E_UNKW_NETWORK;
		goto err_out_2;
	}

	len = s.data_buffer.length;
	list->list = (char *)malloc(len + 1);
	if (!list->list) {
		ret = EUPD_E_NO_MEMORY;
		goto err_out;
	}
	memcpy(list->list, s.data_buffer.data, len);
	list->list[len] = '\0';

	destroy_session(&s);

	return EUPD_OK;

err_out_2:
	len = strlen(s.error_string);
	list->error_string = (char *)malloc(len + 1);
	if (!list->error_string) {
		ret = EUPD_E_NO_MEMORY;
		goto err_out;
	}
	strcpy(list->error_string, s.error_string);
err_out:
	destroy_session(&s);
	return ret;
}

void fetcher_init(void)
{
	curl_global_init(CURL_GLOBAL_DEFAULT);
}

void fetcher_list_cleanup(struct DownloadedList *list)
{
	free(list->list);
	free(list->error_string);
}
