This is a WIP! It is not usable.

It is currently very rough, unformatted, undocumented, and a moving target.

TODO: Fix those things.

Interesting features:

- Establishes connections to the Bitcoin network and others like it.
- Attempts to maintain as many incoming/outgoing connections as are allowed
  by the connection parameters.
- Works threaded or unthreaded.
- Fully async dns resolving.
- Fully async SOCKS5 support.
- Supports RESOLVE command for Tor DNS queries over SOCKS5.
- Accepts/Connects via IP or Unix domain sockets.
- Per-connection and per-group rate-limiting.
- Able to connect to different networks simultaneously.
- Generally attempts to mimic Bitcoin Core network behavior.
- Allows each connection to define connection attempts, persistent reconnects, etc.
- Does not leak networking behavior out of the library! Library users see only
  connection ids, so network behavior is abstracted away.
- Other fun stuff.

See the samples dir to get an idea of how things work.

First TODO: A design doc explaining the approaches taken (and many more that
 were tried and discarded).
