# simple-web-proxy
A simple web proxy server capable of accepting multiple HTTP GET requests from clients, pass them to HTTP server and handle returning traffic from the HTTP server back to clients. In addition to this it also supports link prefetching to speed up loading process for subsequent GET requests.

## Compiling
MakeFile Targets:
- `proxy`: Compiling code in `bin` folder
- `clean`: Clean `cache` and `bin` folders
- `run` : Start the webproxy with `PORT` and `TIMEOUT`
- `kill` : Stop existing process at same `PORT`

## Design Decisions
- One thread per request and a new connection is opened each time, in case page is not present in the cache
- Separate threads for page pre-fetching so as not to block the current request serving
- One single main thread for accepting new requests concurrently which then spawns another thread to handle the requests
- All the hosts/ip to be blocked can be placed in `BLOCKED` file. Requests sent to any of these remotes will result in `403 Forbidden` Response from proxy
