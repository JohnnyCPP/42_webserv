# Future tests — Phase 3 (response generation)

This file is the checklist for tests that **cannot pass yet** because the
request-routing layer is not implemented. Today,
`WebServer::processClientRequest()` ignores the method/path/config and always
returns a hardcoded `200 "Hello from webserv!"` page. Every behaviour below
depends on replacing that stub with real routing.

Each item maps to a line on the 42 evaluation sheet
(`document/evaluation/ng_5_webserv.pdf`). The functional runner
(`tests/functional/run.sh`) already issues most of these requests in its
`PENDING` section — promote each one from `pending()` to a hard assertion
(`assert_eq` / `assert_contains`) as the feature lands.

## Eval: Configuration

- [ ] **Multiple servers, different ports** — partially active (different
      *server blocks* on different ports already work). Still TODO: serve a
      *different website* per port and assert the body differs.
- [ ] **Multiple servers, different hostnames** — route by `server_name`.
      `curl --resolve name:port:127.0.0.1 http://name:port/` must reach the
      matching server block; an unknown host falls back to the first server.
- [ ] **Custom error page** — `error_page 404 /errors/404.html` must serve that
      file's contents (not the built-in default body) with status 404.
- [ ] **client_max_body_size** — POST a body larger than the limit → `413`;
      a body within the limit → accepted. Test both sides of the boundary.
- [ ] **Routes to different directories** — `location /public { root ... }`
      must serve from the location root, not the server root.
- [ ] **Default file for a directory** — requesting a directory serves its
      `index` file; with no index and `autoindex off` → `403`/`404`.
- [ ] **Per-route allowed methods** — a method not in `allow_methods` → `405`
      with an `Allow` header. Test "delete with and without permission".

## Eval: Basic checks

- [ ] **GET** returns the requested resource with `200` and correct body.
- [ ] **POST** to an upload route stores the body and returns `201`/`200`.
- [ ] **DELETE** removes a target file and returns `200`/`204`; deleting a
      missing target → `404`.
- [x] **UNKNOWN method does not crash** — already covered (active).
- [ ] **Every status code correct** — partially covered by the HttpResponse
      unit tests; needs end-to-end assertions per route.
- [ ] **Upload a file and get it back** — POST a file to `upload_store`, then
      GET it and compare bytes (round-trip).

## Eval: Browser / static site

- [ ] Serve a fully static website (HTML + linked CSS/JS/images) with correct
      `Content-Type` per asset.
- [ ] Wrong URL → `404`.
- [ ] Directory listing (`autoindex on`) produces a navigable HTML index.
- [ ] Redirected URL (`return 301 /target`) → `301` + `Location` header.

## Eval: Port issues

- [ ] **Same port declared twice in one config must fail to start.** Today the
      duplicate-port check only happens at the OS bind level across separate
      processes (active test covers that). A single config with two identical
      `listen` directives needs explicit detection / a clean failure.
- [ ] Multiple ports each serve their configured site (browser check).

## Eval: Siege & stress

- [ ] `siege -b` on an empty page → availability > 99.5%.
- [ ] No unbounded memory growth (monitor RSS over a long `siege -b`).
- [ ] No hanging connections (the active suite has a light 100-request proxy;
      replace/augment with real `siege` once routing returns real pages).
- [ ] Runs indefinitely without a restart.
- [ ] Run under `valgrind --leak-check=full` with no leaks at shutdown
      (`make valgrind` target exists).

## Eval: CGI (bonus)

- [ ] Execute a CGI script (`.py` / `.php`) for GET and POST; pass the body on
      stdin and env vars (`REQUEST_METHOD`, `QUERY_STRING`, `CONTENT_LENGTH`…).
- [ ] CGI output (its headers + body) is forwarded to the client.
- [ ] The repo ships demo scripts under `www/cgi-bin/{python,php}` and the
      config wires `cgi_extension`, but no CGI execution exists yet.

---

## Bugs / gaps found while writing these tests

These are reception-layer issues independent of Phase 3 — worth raising with
Johnny:

1. **Second `listen` in a server block is ignored.** `Server::bindSocket()`
   only binds `config.getListenAddresses()[0]`, so `complete.conf`'s
   `listen 127.0.0.1:8081;` never opens (verified: port 8081 stays closed). A
   `Server` needs one listening socket per `listen` directive.
2. **`make test` is broken on `main`.** The target compiles
   `src/request.cpp`, which the merge dropped, and `include/request.hpp` uses
   `BodyLengthStatus` / `RequestStatus` types that are now defined nowhere. The
   old standalone parser API is dead — the live parser is inside `Client.cpp`.
   Either delete `include/request.hpp` + the `test` target, or repoint it at
   the new unit tests under `tests/unit/`.
3. **Configured error-page files don't exist.** `complete.conf` /
   `test.conf` reference `/errors/404.html` etc., but there is no
   `www/demo/errors/` directory. Custom-error-page tests need those files
   created first.
4. **Keep-alive is stubbed off.** `WebServer::handlePollOut` always closes
   after one response (`keepAlive = false`). Fine for now, but browsers and
   `siege` reuse connections — revisit before the stress test.
