# TCP over HTTP

This is TCP over HTTP

## Why?

My school filters the internet quite a bit, but they're weird about it.

* HTTPS is completely filtered. The network has a custom signing authority that
allows them to view and filter any TLS data they see.
* Many other ports are completely banned, most notably port 22 (SSH).
* The school can detect if you're using a VPN, and will disable your WiFi for 5
minutes if you do. Tor does work with bridges, although it's quite slow and
probably not very good for the network.
* The school will also block protocols, not just ports, so moving SSH to some
other port won't work.
* Despite this, however, many protocols remain unblocked. Most notably, VNC,
which would allow you to connect directly to a home desktop as long as you're
fine with the school district being able to see your screen and monitor your
keystrokes at all times.
* In addition, many other ports are completely unfiltered. Port 25 (can be used
for email spam) is allowed, as is port 70 (Gopher), port 1965 (Gemini), and most
importantly, port 80 (HTTP).

This means that if we can encode a VPN into one of these unblocked protocols, we
can bypass all the filters with a fast internet connection. This repo obviously
encodes TCP into HTTP.

## How does it work?

We're not really trying to write a decent web server that follows all of the
HTTP paradigms, we're trying to encode data. That's why this program sends two
requests simultaneously and nothing else. One request sends data, the other
receives.

The sending socket sends a POST request that looks like this:

    POST / HTTP/1.1
    Host: [CONFIG_HOST]
    Content-Length: 9223372036854775807

and the receiving socket sends a GET request that looks like this:

    GET / HTTP/1.1
    Host: [CONFIG_HOST]

The server responds to that request with this response:

    HTTP/1.1 200 OK
    Content-Length: 9223372036854775807

Now, the client can send data through the POST request and the server can send
data back through the GET request. We've created a TCP connection with the
server that can send any data including SSH connections.

Of course, having a TCP connection is useless if it's confined to code that you
write, we have to be able to forward this data to other programs. Sending data
from the server side to somewhere else is pretty straightforward, you just
specify a host and a port, and you can relay data between two hosts.

The client side is a bit harder. The solution I came up with was to create
another server on the client side that some other program like ssh can connect
to. ssh connects to a server on the client machine, which forwards that data via
HTTP to the other server, which forwards that data again to sshd.

My Dad (quite reasonably) got confused after hearing me explain this, so here's
a diagram I made.

![Diagram explaining this cursed configuration](https://raw.githubusercontent.com/NateChoe1/tcp-over-http/master/diagram.png)

## How do I configure this

Change `config.h`. I blame [suckless](https://suckless.org) for this.
