/* The host that gets sent in HTTP requests */
#define CONFIG_HTTP_HOST "proxy.natechoe.dev"

#define CONFIG_CLIENT_PORT 9000

#define CONFIG_SERVER_HOST "127.0.0.1"
#define CONFIG_SERVER_PORT 9001

/* Change these to the destination of the proxy */
#define CONFIG_DEST_HOST "127.0.0.1"
#define CONFIG_DEST_PORT 9002

/* Ideally these are absolute paths, but not every system is the same. */
#define CONFIG_CLIENT_NC "nc"
#define CONFIG_SERVER_NC "nc"
