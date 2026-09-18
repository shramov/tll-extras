#include <tll/config.h>
#include <tll/logger.h>
#include <tll/util/string.h>
#include <tll/util/url.h>

#include <tll/channel/module.h>

#include <zip.h>

template <> struct std::default_delete<zip_t> { void operator ()(zip_t *ptr) const { zip_discard(ptr); } };
template <> struct std::default_delete<zip_error_t> { void operator ()(zip_error_t *ptr) const { zip_error_fini(ptr); } };

tll_config_t * zipload(const char * cpath, int plen, void *)
{
	auto log = tll::Logger("tll.config.zip");
	std::string zpath, ipath;
	{
		auto data = tll::string_view_from_c(cpath, plen);
		auto sep = data.find("::");
		if (sep == data.npos)
			return log.fail(nullptr, "Missing '::' separator in '{}'", data);
		zpath = data.substr(0, sep);
		ipath = data.substr(sep + 2);
	}
	int error = 0;
	auto zip = zip_open(zpath.c_str(), ZIP_RDONLY, &error);
	if (!zip) {
		zip_error_t err;
		zip_error_init_with_code(&err, error);
		std::unique_ptr<zip_error_t> guard { &err };
		return log.fail(nullptr, "Failed to open zip file {}: {}", zpath, zip_error_strerror(&err));
	}

	std::unique_ptr<zip_t> guard{zip};

	zip_stat_t stat = {};
	if (auto r = zip_stat(zip, ipath.c_str(), 0, &stat); r) {
		auto err = zip_get_error(zip);
		return log.fail(nullptr, "No file '{}' in zip file {}: {}", ipath, zpath, zip_error_strerror(err));
	}

	if (stat.size > 32 * 1024 * 1024) // Limit config size to 32mb
		return log.fail(nullptr, "Config size too large: {}", stat.size);

	std::vector<char> buf;
	buf.resize(stat.size);

	auto fp = zip_fopen_index(zip, stat.index, 0);
	if (!fp)
		return nullptr;

	auto r = zip_fread(fp, buf.data(), buf.size());
	zip_fclose(fp);
	if (r != (ssize_t) buf.size())
		return nullptr;

	guard.reset();
	std::string_view data { buf.data(), buf.size() };
	if (data.substr(0, 11) == "yamls+gz://") {
		while (data.back() == '\n')
			data = data.substr(0, data.size() - 1);
	}
	if (data.substr(0, 8) == "yamls://" || data.substr(0, 11) == "yamls+gz://")
		return tll_config_load(data.data(), data.size());
	return tll_config_load_data("yamls", -1, data.data(), data.size());
}

static int modinit(struct tll_channel_module_t * m, tll_channel_context_t * ctx, const tll_config_t * cfg)
{
	return tll_config_load_register("zip", -1, zipload, nullptr);
}

static int modfree(struct tll_channel_module_t * m, tll_channel_context_t * ctx)
{
	tll_config_load_unregister("zip", -1, zipload, nullptr);
	return 0;
}

static tll_channel_module_t mod = {
	.version = TLL_CHANNEL_MODULE_VERSION,
	.impl = nullptr,
	.init = modinit,
	.free = modfree,
};

extern "C" tll_channel_module_t * tll_channel_module()
{
	return &mod;
}
