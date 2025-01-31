#ifndef INCLUDE_CONFIG_H
#define INCLUDE_CONFIG_H

#define PROJECT_NAME "Kedge"
#define PROJECT_VER  "0.0.2"
#define PROJECT_VER_MAJOR "0"
#define PROJECT_VER_MINOR "0"
#define PTOJECT_VER_PATCH "2"

#ifdef __APPLE__
#define APP_NAME "Kedge"
#else
#define APP_NAME "kedge"
#endif

#define SERVER_SIGNATURE "Kedge/0.0.2"

#define ENV_PEERID_PREFIX "LT_PEERID_PREFIX"
#define ENV_USER_AGENT "LT_USER_AGENT"
#define ENV_HANDSHAKE_CLIENT_VERSION "LT_HANDSHAKE_CLIENT_VERSION"
#define ENV_UPLOAD_RATE "LT_UPLOAD_RATE"
#define ENV_CONNECTIONS_LIMIT "LT_CONNECTIONS_LIMIT"
#define ENV_LISTEN_ADDR "LT_LISTEN_ADDR"
#define ENV_BOOTSTRAP_NODES "DHT_BOOTSTRAP_NODES"
#define ENV_WEBUI_ROOT "KEDGE_WEBUI_ROOT"
#define ENV_STORE_ROOT "KEDGE_STORE_ROOT"
#define ENV_MOVED_ROOT "KEDGE_MOVED_ROOT"
#define ENV_HTTP_ADDR "KEDGE_HTTP_ADDR"
#define ENV_HTTP_PORT "KEDGE_HTTP_PORT"


#endif // INCLUDE_CONFIG_H
