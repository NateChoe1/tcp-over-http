# TCP over HTTP

This is TCP over HTTP

## Why?

My school restricts internet access quite a bit. HTTPS websites are restricted
by default, but the people who run the filtering spent all of their time
worrying about HTTPS and forgot to disable HTTP. Those idiots did not expect
some bigger, smarter idiot to implement TCP over HTTP.

## How does it work?

Because the semantics of HTTP aren't really important, this is not a stateless
protocol. Instead, the client opens two socket connections to the server. One
sends packets, the other receives them.

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

Now we effectively have a socket connection. We can send data on one socket and
receive from another. We then need to make sure that the two connections were
sent correctly, so the client sends a random 4 byte sequence and the server
returns that same sequence back. If the sequences are different, then
something's gone wrong and the client stops the connections.

The client program now generates a two way pipe between some process and the
server. If this other process is netcat, then you can connect to netcat with
some third process like SSH.

## How can I use this

This is quite useless unless you have some other program to run it with. You can
create a double pipe between the client program and netcat listening on some
port, and then connect another third program to that port. The third program
sends data to netcat, which sends it to this program, which sends it to the
server, which forwards that data to some other place, which responds with some
data, which the server forwards to the client, which the client forwards to
netcat, which forwards it to the other program.

## How do I configure this

Change `config.h`. I blame [suckless](https://suckless.org) for this.
