

#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <unordered_set>

#include <boost/json/value.hpp>
namespace json = boost::json;
#include <boost/json/serialize.hpp>
#include "json_diff.hpp"

#include <libtorrent/config.hpp>

#include "log.hpp"
#include "handlers.hpp"
#include "http_websocket.hpp"

namespace btd {

httpCaller::
httpCaller(std::shared_ptr<sheath> const& shth,
	std::string ui_dir)
	 : shth_(shth)
	 , ui_root_(ui_dir)
{}


const std::optional<http::response<string_body>>
httpCaller::
sbCall(http::request<string_body> const& req)
{
	if (req.target().find("/api/") == std::string_view::npos) return std::nullopt;
	auto uri = req.target().substr(4);
	if (req.method() == verb::get && uri == "/session/info"sv) return handleSessionInfo(req);
	if (req.method() == verb::get && uri == "/session/stats"sv) return handleSessionStats(req);
	if (req.method() == verb::get && uri == "/sync/stats"sv) return handleSyncStats(req);
	if (uri == "/torrents"sv) return handleTorrents(req);

    if (uri.find("/torrent/") == 0) return handleTorrent(req, 13); // len(/api/torrent/) == 13

	// TODO: more routes
	return std::nullopt;
}

http::response<string_body>
httpCaller::
handleSessionInfo(http::request<string_body> const& req)
{
    return make_resp<string_body>(req, json::serialize(shth_->getSessionInfo()), ctJSON);
}

http::response<string_body>
httpCaller::
handleSessionStats(http::request<string_body> const& req)
{
	return make_resp<string_body>(req, json::serialize(shth_->getSessionStats()), ctJSON);
}

http::response<string_body>
httpCaller::
handleSyncStats(http::request<string_body> const& req)
{
	return make_resp<string_body>(req, json::serialize(shth_->getSyncStats()), ctJSON);
}

http::response<string_body>
httpCaller::
handleTorrents(http::request<string_body> const& req) // get or post
{
	if (req.method() == verb::get)
	{
		return make_resp<string_body>(req, json::serialize(shth_->get_torrents()), ctJSON);
	}

	if (req.method() == verb::post)
	{
		LOG_DEBUG << " post clen " << req.find(http::field::content_length)->value() << " bytes\n";
		auto savePath = req.find("x-save-path");
		if (savePath == req.end())
		{
			return make_resp_400(req, "miss save-path");
		}
		const char* buf = req.body().data();
		const int size = req.body().size();
		const std::string dir(savePath->value());
		LOG_DEBUG << "post metainfo " << size << " bytes with save-path: '" << dir << "'\n";
		if (shth_->add_torrent(buf, size, dir))
		{
			return make_resp_204(req);
		}
		return make_resp_500(req, "failed to add");
	}

	return make_resp_400(req, "Unsupported method");
}


// handle a torrent with GET or POST or DELETE
http::response<string_body>
httpCaller::
handleTorrent(http::request<string_body> const& req, size_t const offset)
{
	// len(/api/torrent/{info_hash}/{act}?) >= 4 + 9 + 40
	const std::string s(req.target().data(), req.target().size());
	const size_t ih_size = 40;
	const size_t min_size = offset+ih_size;
	if (s.size() < min_size) return make_resp_404(req);

	lt::sha1_hash ih;
	if (!from_hex(ih, s.substr(offset, ih_size)))
	{
		return make_resp_400(req, "invalid hash string");
	}
	if (req.method() == verb::head)
	{
		if (shth_->exists(ih))
		{
			return make_resp_204(req);
		}
		return make_resp_404(req);
	}
	std::string act = "";
	if (s.size() > min_size + 1)
	{
		act = s.substr(min_size + 1);
	}
	if (req.method() == verb::get)
	{
		auto flag = sheath::query_basic;
		if ("peers" == act) flag = sheath::query_peers;
		else if ("files" == act) flag = sheath::query_files;
		auto jv = shth_->get_torrent(ih, flag);
		if (jv.is_null()) { return make_resp_404(req); }
		return make_resp<string_body>(req, json::serialize(jv), ctJSON);
	}
	if (req.method() == verb::delete_)
	{
		shth_->drop_torrent(ih, ("yes" == act || "with_data" == act));
		return make_resp_204(req);
	}

	return make_resp_400(req, "Unsupported method");
}

void
httpCaller::
join(websocket_session* wss)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.insert(wss);
    sync_ver ++;
    auto qid = wss->qid();
    if (qid.empty()) qid = n2hex(randNum(1000, 9999));
    if (curr_stats.is_null()) curr_stats = getSyncStats();
    json::value jv({
        {"version", sync_ver.load()}
        ,{"id", qid}
        ,{"body", curr_stats}
    });
    LOG_DEBUG << "ws joinning " << sync_ver.load();
    wss->send(std::make_shared<std::string const>(json::serialize(jv)));
}

void
httpCaller::
leave(websocket_session* wss)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sessions_.erase(wss);
    LOG_DEBUG << "ws leaved";
}

// Broadcast a message to all websocket client sessions
void
httpCaller::
broadcast(std::string message)
{
    // Put the message in a shared pointer so we can re-use it for each client
    auto const ss = std::make_shared<std::string const>(std::move(message));

    // Make a local list of all the weak pointers representing
    // the sessions, so we can do the actual sending without
    // holding the mutex:
    std::vector<std::weak_ptr<websocket_session>> vws;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        vws.reserve(sessions_.size());
        for(auto p : sessions_)
            vws.emplace_back(p->weak_from_this());
    }

    // For each session in our local list, try to acquire a strong
    // pointer. If successful, then send the message on that session.
    for(auto const& wp : vws)
        if(auto sp = wp.lock())
            sp->send(ss);
}

void
httpCaller::
closeWS()
{
    std::lock_guard<std::mutex> lock(mutex_);
	for(auto p : sessions_)
		p->close("close by caller");
}

json::value
httpCaller::
getSyncStats()
{
	return shth_->getSyncStats();
}

void
httpCaller::
doLoop()
{
    if (sessions_.empty())
    {
    	std::this_thread::sleep_for(std::chrono::seconds(20));
    	return;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    sync_ver ++;
    curr_stats = shth_->getSyncStats();
    json::value jv({
        {"version", sync_ver.load()}
        ,{"delta", true}
        ,{"body", diff(prev_stats, curr_stats)}
    });
    broadcast(json::serialize(jv));
    // std::clog << sync_ver.load() << " ";
	prev_stats = curr_stats;

}

} // namespace btd
