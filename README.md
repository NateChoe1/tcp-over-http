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

From RFC 9110:

> All responses, regardless of the status code (including interim responses) can
> be sent at any time after a request is received, even if the request is not
> yet complete.  A response can complete before its corresponding request is
> complete (Section 6.1).  Likewise, clients are not expected to wait any
> specific amount of time for a response.  Clients (including intermediaries)
> might abandon a request if the response is not received within a reasonable
> period of time.

This means HTTP servers can send a response before the client even finishes
sending the request, so this interaction is perfectly valid:

    > GET / HTTP/1.1
    < HTTP/1.1 500 Internal Server Error
    < Content-Length: 13
    < 
    < We screwed up
    > Host: example.com
    > 

As is this one:

    > POST / HTTP/1.1
    > Host: example.com
    > Content-Length: 9223372036854775807
    >
    < HTTP/1.1 500 Internal Server Error
    < Content-Length: 9223372036854775807
    <

Note that after this interaction, the client is both sending and receiving a
message body. Any new data sent over this TCP stream can be interpreted as
perfectly valid HTTP.

Here's an outdated diagram explaining how this works:

![Diagram explaining this configuration](https://raw.githubusercontent.com/NateChoe1/tcp-over-http/master/diagram.png)

Note that the "HTTP GET request" at the bottom of the diagram ought to say "500
Interal Server Error response", I just don't feel like changing it right now.

## How do I configure this

Change `config.h`. I blame [suckless](https://suckless.org) for this.
