#include "list_parser.h"
#include "json.hpp"
#include "list_comparator.h"

#include <cstring>
#include <echmetupdatecheck.h>

#ifdef EUPD_ENABLE_DIAGNOSTICS
#include <iostream>
#endif // EUPD_ENABLE_DIAGNOSTICS

typedef nlohmann::json json_t;
typedef nlohmann::json::parse_error parse_error_t;

static const std::string LINK("link");
static const std::string MAJOR("major");
static const std::string MINOR("minor");
static const std::string NAME("name");
static const std::string REVISION("revision");
static const std::string SEVERITY("severity");
static const std::string SOFTWARE("software");
static const std::string VERSIONS("versions");

class InvalidItemError : public std::runtime_error {
public:
	using std::runtime_error::runtime_error;
};

static
bool has_entry(const json_t &o, const std::string &key)
{
	return o.find(key) != o.end();
}

static
bool is_json_revision_valid(const std::string &rev)
{
	return is_revision_valid(rev.c_str(), rev.length());
}

static
bool is_json_software_valid(const json_t &sw)
{
	if (!sw.is_string())
		return false;

	const auto &name = sw.get<std::string>();
	if (name.length() > STRUCT_MEM_SZ(struct Software, name))
		return false;
	return is_software_name_valid(name.length());
}

static
bool is_version_item(const json_t &o)
{
	if (!has_entry(o, MAJOR))
		return false;
	if (!o[MAJOR].is_number())
		return false;

	if (!has_entry(o, MINOR))
		return false;
	if (!o[MINOR].is_number())
		return false;

	if (!has_entry(o, REVISION))
		return false;

	const auto &rev = o[REVISION];
	if (!rev.is_string())
		return false;
	if (!is_json_revision_valid(rev.get<std::string>()))
		return false;

	if (!has_entry(o, SEVERITY))
		return false;
	const auto sev = o[SEVERITY];
	return (sev >= 0 && sev < 3);
}

static
void check_item(const json_t &item)
{
	if (!item.is_object())
		throw InvalidItemError("Item is not a JSON object");

	if (!has_entry(item, NAME))
		throw InvalidItemError("Item does not have \"name\" field");
	if (!is_json_software_valid(item[NAME]))
		throw InvalidItemError("Software name is invalid");

	if (!has_entry(item, LINK))
		throw InvalidItemError("Item does not have \"link\" field");
	const auto &link = item[LINK];
	if (!link.is_string())
		throw InvalidItemError("Field \"link\" is not a string");
	if (link.get<std::string>().length() < 1)
		throw InvalidItemError("Field \"link\" is an empty string\"");

	if (!has_entry(item, VERSIONS))
		throw InvalidItemError("Item does not have \"versions\" field");
	const auto &vers = item[VERSIONS];
	if (!vers.is_array())
		throw InvalidItemError("Field \"versions\" is not an array");
	if (vers.size() < 1)
		throw InvalidItemError("Field \"versions\" is empty");
}

static
void parse_version(const json_t &v, struct ListVersion *lv)
{
	if (!is_version_item(v))
		throw InvalidItemError("Malformed version object");

	const auto major = v[MAJOR].get<int>();
	const auto minor = v[MINOR].get<int>();
	const auto &rev = v[REVISION].get<std::string>();
	const auto sev = v[SEVERITY].get<int>();

	lv->version.major = major;
	lv->version.minor = minor;
	std::strncpy(lv->version.revision, rev.c_str(), STRUCT_MEM_SZ(struct EUPDVersion, revision));
	lv->severity = [sev]() {
		switch (sev) {
		case 0:
			return SEV_FEATURE;
		case 1:
			return SEV_BUGFIX;
		case 2:
			return SEV_CRITICAL;
		default:
			throw std::logic_error("Invalid severity value - sanity check failure");
		}
	}();
}

static
void parse_item(const json_t &item, struct Software *sw)
{
	check_item(item);

	const auto &name = item[NAME].get<std::string>();
	const auto &link = item[LINK].get<std::string>();
	const auto &vers = item[VERSIONS];

	std::memset(sw, 0, sizeof(struct Software));

	const size_t vers_sz = sizeof(struct ListVersion) * vers.size();
	sw->versions = static_cast<ListVersion *>(malloc(vers_sz));
	if (sw->versions == nullptr)
		throw std::bad_alloc();
	std::memset(sw->versions, 0, vers_sz);

	const size_t link_len = link.length();
	sw->link = static_cast<char *>(malloc(link_len + 1));
	if (sw->link == nullptr) {
		free(sw->versions);

		throw std::bad_alloc();
	}

	std::strncpy(sw->name, name.c_str(), STRUCT_MEM_SZ(struct Software, name));
	std::strncpy(sw->link, link.c_str(), link_len + 1);

	size_t idx;
	for (idx = 0; idx < vers.size(); idx++) {
		const auto &v = vers[idx];
		struct ListVersion *lv = &sw->versions[idx];

		try {
			parse_version(v, lv);
		} catch (const InvalidItemError &) {
			free(sw->link);
			free(sw->versions);

			sw->num_versions = idx;

			throw;
		}
	}
	sw->num_versions = idx;
}

static
EUPDRetCode walk_list(const json_t &root, struct SoftwareList *sw_list)
{
	EUPDRetCode tRet;

	if (!root.is_object())
		return EUPD_E_MALFORMED_LIST;
	if (!has_entry(root, SOFTWARE))
		return EUPD_E_MALFORMED_LIST;

	const auto &software = root[SOFTWARE];
	if (!software.is_array())
		return EUPD_E_MALFORMED_LIST;

	sw_list->items = static_cast<struct Software *>(calloc(software.size(), sizeof(struct Software)));
	if (sw_list->items == nullptr)
		return EUPD_E_NO_MEMORY;

	size_t idx = 0;
	for (const auto &item : software) {
		try {
			parse_item(item, &sw_list->items[idx]);
		} catch (const InvalidItemError &ex) {
		#ifdef EUPD_ENABLE_DIAGNOSTICS
			std::cerr << "Bad item in list: " << ex.what() << std::endl;
		#else
			(void)ex;
		#endif // EUPD_ENABLE_DIAGNOSTICS

			tRet = EUPD_W_LIST_INCOMPLETE;
			goto out;
		} catch (const std::bad_alloc &) {
			tRet = EUPD_W_LIST_INCOMPLETE;
			goto out;
		}
		idx++;
	}

	tRet = EUPD_OK;
out:
	sw_list->length = idx;

	return tRet;
}

extern "C" {

void parser_free_list(struct SoftwareList *sw_list)
{
	for (size_t idx = 0; idx < sw_list->length; idx++) {
		const auto sw = &sw_list->items[idx];
		free(sw->link);
		free(sw->versions);
	}

	free(sw_list->items);
}

EUPDRetCode parser_parse(const char *list_string, struct SoftwareList *sw_list)
{
	json_t j;

	try {
		j = json_t::parse(list_string);
	} catch (const parse_error_t &) {
		return EUPD_E_MALFORMED_LIST;
	}

	return walk_list(j, sw_list);
}

EUPDRetCode parser_set_link(const struct SoftwareList *sw_list, const char *name, struct EUPDResult *result)
{
	static const size_t name_len = STRUCT_MEM_SZ(struct Software, name);

	for (size_t idx = 0; idx < sw_list->length; idx++) {
		const auto sw = &sw_list->items[idx];

		if (!STRNICMP(name, sw->name, name_len)) {
			result->link = static_cast<char *>(malloc(strlen(sw->link) + 1));
			if (result->link == nullptr)
				return EUPD_E_NO_MEMORY;
			std::strcpy(result->link, sw->link);

			return EUPD_OK;
		}
	}

	abort(); /* This cannot happen */
}

}
