// SPDX-License-Identifier: MIT
// SPDX-FileCopyrightText: Pavel Shramov <shramov@mexmat.net>

#include <tll/channel/module.h>
#include <tll/channel/prefix.h>
#include <tll/scheme/merge.h>
#include <tll/util/mmstruct.h>

static constexpr std::string_view restore_scheme = "yamls://[{name: Ack, id: 80}]";
static constexpr int control_ack_msgid = 80;

class Restore : public tll::channel::Prefix<Restore>
{
	using Base = tll::channel::Prefix<Restore>;
	tll::scheme::ConstSchemePtr _scheme_control_child;
	tll::scheme::ConstSchemePtr _scheme_control_init;

	std::string _filename;
	tll::util::MMStruct<int64_t> _mmseq;

 public:
	static constexpr std::string_view channel_protocol() { return "restore+"; }

	const tll::Scheme * scheme(int type) const
	{
		if (type == TLL_MESSAGE_CONTROL)
			return _scheme_control.get();
		return Base::scheme(type);
	}

	int _init(const tll::Channel::Url &url, tll::Channel *master);
	int _open(const tll::ConstConfig &cfg);

	int _post(const tll_msg_t *msg, int flags);

	int _init_control(const tll::Scheme * child);

	int _on_active();
	int _on_closed()
	{
		_mmseq.reset();
		return Base::_on_closed();
	}
};

int Restore::_init(const tll::Channel::Url &url, tll::Channel *master)
{
	if (auto r = Base::_init(url, master); r)
		return r;

	auto reader = channel_props_reader(url);
	_filename = reader.getT<std::string>("file");
	if (!reader)
		return _log.fail(EINVAL, "Invalid url: {}", reader.error());

	if (auto r = _scheme_load(restore_scheme, TLL_MESSAGE_CONTROL); r)
		return _log.fail(r, "Failed to load control scheme");
	_scheme_control_init = std::move(_scheme_control);
	_scheme_control_child.reset(tll_scheme_ref(_child->scheme(TLL_MESSAGE_CONTROL)));


	if (auto r = _init_control(_scheme_control_child.get()); r)
		return r;

	return 0;
}

int Restore::_init_control(const tll::Scheme * child)
{
	if (child) {
		if (_scheme_control_init) {
			auto merged = tll::scheme::merge({_scheme_control_init.get(), child});
			if (!merged)
				return _log.fail(EINVAL, "Failed to merge control scheme with child: {}", merged.error());
			_scheme_control.reset(*merged);
		} else
			_scheme_control.reset(child->ref());
	} else
		_scheme_control.reset(_scheme_control_init->ref());

	return 0;
}

int Restore::_on_active()
{
	if (auto control = _child->scheme(TLL_MESSAGE_CONTROL); control != _scheme_control_child.get()) {
		if (auto r = _init_control(control); r)
			return _log.fail(r, "Failed to initialize control scheme");
		_scheme_control_child.reset(control->ref());
	}

	return Base::_on_active();
}

int Restore::_open(const tll::ConstConfig &)
{
	if (auto r = _mmseq.init(_filename); !r)
		return _log.fail(EINVAL, "Failed to open mmseq file '{}', {}: {}", _filename, r.message, strerror(r.err));
	tll::Config ocfg;
	if (*_mmseq > 0) {
		ocfg.set("mode", "seq");
		ocfg.setT("seq", *_mmseq);
	} else {
		ocfg.set("mode", "initial");
	}
	return Base::_open(ocfg);
}

int Restore::_post(const tll_msg_t *msg, int flags)
{
	if (msg->type == TLL_MESSAGE_CONTROL && msg->msgid == control_ack_msgid) {
		*_mmseq = msg->seq + 1;
		return 0;
	}
	return Base::_post(msg, flags);
}

TLL_DEFINE_IMPL(Restore);

TLL_DEFINE_MODULE(Restore);
