#pragma once

#include <string>

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <libtorrent/config.hpp>
#include <libtorrent/session.hpp>

#include "const.hpp"
#include "util.hpp"

namespace btd {

// Forward declaration
class sheath;

void
load_sess_params(std::string const& cd, lt::session_params& params);

struct Option {

    std::string peerID    = "";
    std::string listens   = "";
    std::string movedRoot = "";
    std::string storeRoot = "";
    std::string userAgent = "";
    std::string handshakeClientVersion = "";
    int connectionsLimit = 0;
    int uploadRate = 0;
    std::string webuiRoot = "";
    std::string httpAddr = "127.0.0.1";
    std::uint_least16_t httpPort = 16180;

	lt::session_params params;

    bool
    init_from(int argc, char* argv[]);

    std::shared_ptr<sheath>
    make_context() const;
};

std::string mapper(std::string env_var)
{
   // ensure the env_var is all caps
   std::transform(env_var.begin(), env_var.end(), env_var.begin(), ::toupper);

   if (env_var == ENV_PEERID_PREFIX || env_var == "TR_PEERID_PREFIX") return "peer-id";
   if (env_var == ENV_BOOTSTRAP_NODES) return "dht-bootstrap-nodes";
   if (env_var == ENV_MOVED_ROOT) return "moved-root";
   if (env_var == ENV_STORE_ROOT) return "store-root";
   if (env_var == ENV_WEBUI_ROOT) return "webui-root";
   if (env_var == ENV_HTTP_ADDR) return "http-addr";
   if (env_var == ENV_HANDSHAKE_CLIENT_VERSION) return "handshake-client-version";
   if (env_var == ENV_CONNECTIONS_LIMIT) return "connections-limit";
   if (env_var == ENV_USER_AGENT) return "user-agent";
   if (env_var == ENV_UPLOAD_RATE) return "upload-rate";
   if (env_var == ENV_LISTEN_ADDR) return "listens";
#ifndef __APPLE__
   if (env_var == ENV_HTTP_PORT) return "http-port";
#endif
   return "";
}

bool
Option::init_from(int argc, char* argv[])
{
    po::options_description config("configuration");
    config.add_options()
        ("help,h", "print usage message")
        ("listens", po::value<std::string>(&listens)->default_value("0.0.0.0:16188"), "listen_interfaces " ENV_LISTEN_ADDR)
        ("moved-root", po::value<std::string>(&movedRoot), "moved root, env: " ENV_MOVED_ROOT)
        ("user-agent", po::value<std::string>(&userAgent)->default_value("qBittorrent/4.3.8"), "libtorrent user agent: " ENV_USER_AGENT)
        ("handshake-client-version", po::value<std::string>(&handshakeClientVersion)->default_value("qBittorrent/4.3.8"),"extended handshake client version: " ENV_HANDSHAKE_CLIENT_VERSION)
        ("connections-limit", po::value<int>(&connectionsLimit)->default_value(500), "connections limit: " ENV_CONNECTIONS_LIMIT)
        ("upload-rate", po::value<int>(&uploadRate)->default_value(10), "upload rate limit: " ENV_UPLOAD_RATE)
        ("store-root,d", po::value<std::string>(&storeRoot)->default_value(getStoreDir()), "store root, env: " ENV_STORE_ROOT)
        ("webui-root", po::value<std::string>(&webuiRoot)->default_value(getWebUI()), "web UI root, env: " ENV_WEBUI_ROOT)
        ("peer-id", po::value<std::string>(&peerID)->default_value("-LT-"), "set prefix of fingerprint, env: " ENV_PEERID_PREFIX)
        ("dht-bootstrap-nodes", po::value<std::string>()->default_value("dht.transmissionbt.com:6881"), "a comma-separated list of Host port-pairs. env: " ENV_BOOTSTRAP_NODES)
        ("http-addr", po::value<std::string>()->default_value("127.0.0.1"), "http listen address, env: " ENV_HTTP_ADDR)
#ifndef __APPLE__
        ("http-port", po::value<std::uint_least16_t>(&httpPort)->default_value(16180), "http listen port, env: " ENV_HTTP_PORT)
#endif
        ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, config), vm);
    po::store(po::parse_environment(config, boost::function1<std::string, std::string>(mapper)), vm);
    notify(vm);

    if (vm.count("help")) {
        std::cout << PROJECT_NAME << " " << PROJECT_VER << "\n";
        std::cout << " Flags and " << config << "\n";
        return false;
    }

    auto conf_dir = getConfDir();
    if (!prepare_dirs(conf_dir)) return false;

    load_sess_params(conf_dir, params);

    using lt::session_handle;
    using lt::settings_pack;

    if (vm.count("user-agent")) {
        LOG_DEBUG << "set user agent " << userAgent;
        params.settings.set_str(settings_pack::user_agent, userAgent);
    }

    if (vm.count("handshake-client-version")) {
        LOG_DEBUG << "set handshake client version " << handshakeClientVersion;
        params.settings.set_str(settings_pack::handshake_client_version, handshakeClientVersion);
    }

    if (vm.count("connections-limit")) {
        LOG_DEBUG << "set connections limit " << connectionsLimit;
        params.settings.set_int(settings_pack::connections_limit, connectionsLimit);
    }

    if (vm.count("upload-rate")) {
        LOG_DEBUG << "set upload rate limit " << uploadRate;
        params.settings.set_int(settings_pack::upload_rate_limit, uploadRate);
    }

    if (vm.count("half-open-limit")) {
        LOG_DEBUG << "set half open connections limit " << halfOpenLimit;
        params.settings.set_int(settings_pack::half_open_limit, halfOpenLimit);
    }

    // config settings and log out
    if (vm.count("listens"))
    {
        LOG_DEBUG << "set listens " << listens;
        params.settings.set_str(settings_pack::listen_interfaces, listens);
    }
    if (vm.count("peer-id"))
    {
        LOG_DEBUG << "set peerID " << peerID;
        params.settings.set_str(settings_pack::peer_fingerprint, peerID);
    }
    if (vm.count("dht-bootstrap-nodes"))
    {
        auto nodes = vm["dht-bootstrap-nodes"].as<std::string>();
        LOG_DEBUG << "set dht-bootstrap-nodes " << nodes;
        params.settings.set_str(settings_pack::dht_bootstrap_nodes, nodes);
    }
    if (vm.count("moved-root"))
    {
        LOG_DEBUG << "set moved root " << movedRoot;
    }
    if (vm.count("store-root"))
    {
        LOG_DEBUG << "set store root " << storeRoot;
    }
    if (vm.count("http-addr"))
    {
    	LOG_DEBUG << "set http addr " << httpAddr;
    }

    return true;
}

std::shared_ptr<sheath>
Option::make_context() const
{
    const auto ses = std::make_shared<lt::session>(std::move(params));
    const auto ctx = std::make_shared<sheath>(ses, storeRoot, movedRoot);
    return ctx;
}

void
load_sess_params(std::string const& cd, lt::session_params& params)
{
    using lt::settings_pack;

    std::vector<char> in;
    if (load_file(path_cat(cd, SESS_FILE), in))
    {
        lt::error_code ec;
        lt::bdecode_node e = lt::bdecode(in, ec);
        lt::session::save_state_flags_t sft = lt::session::save_settings;
#ifndef TORRENT_DISABLE_DHT
        params.dht_settings.privacy_lookups = true;
        sft |= lt::session::save_dht_state;
#endif

        if (!ec) params = read_session_params(e, sft);
    }

    auto& settings = params.settings;

    settings.set_int(settings_pack::alert_mask
        , lt::alert_category::error
        | lt::alert_category::peer
        | lt::alert_category::port_mapping
        | lt::alert_category::storage
        | lt::alert_category::tracker
        | lt::alert_category::connect
        | lt::alert_category::status
        | lt::alert_category::ip_block
        | lt::alert_category::performance_warning
        | lt::alert_category::dht
        // | lt::alert_category::session_log
        // | lt::alert_category::torrent_log
        | lt::alert_category::incoming_request
        | lt::alert_category::dht_operation
        | lt::alert_category::port_mapping_log
        | lt::alert_category::file_progress);

    settings.set_bool(settings_pack::enable_upnp, true);
    settings.set_bool(settings_pack::enable_natpmp, true);
    settings.set_bool(settings_pack::enable_dht, true);
    settings.set_bool(settings_pack::enable_lsd, true);


    settings.set_bool(settings_pack::validate_https_trackers, false);
}

} // namespace btd
