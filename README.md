# netcat-udp

Based on [netcat](https://github.com/openbsd/src/blob/master/usr.bin/nc/netcat.c) sources, it permit to sends the stdin content through UDP frames (client), or wait for UDP frames and print out over stdout (server).

## Build

Use `make`.

```bash
red@DESKTOP-ATO322N:~/projects/netcat-udp$ make
cc -Wall -O -c main.c
cc -o ncu main.o
```

Note: a different target (`debug`) could be used in order to output debug statements.

## Usage

```bash
Usage: ./ncu [HOST] PORT
```

## Examples

You can send some messages by hand (server should be started first...):

- Client (sender):
```bash
echo "Hello World" | ./ncu 127.0.0.1 2031
./ncu 127.0.0.1 2031 < hello.txt
```

- Server (receiver):
```bash
./ncu 2031
Hello World
```

Send a file and display it:

- Client (sender):
```bash
echo "Hello world" > hello.txt
./ncu 127.0.0.1 2031 < hello.txt
```

- Server (receiver):
```bash
./ncu 2031
Hello World
```

Or redirect the output in order to write in a file:

- Client (sender):
```bash
echo "Hello world" > hello.txt
./ncu 127.0.0.1 2031 < hello.txt
```

- Server (receiver):
```bash
./ncu 2031 > hello.txt
```

## TODO
- Check write() return value in order to see if the amount of data written match with the one read.
- Update the buffer size (currently set to 32 bytes) to something around the MTU.

## References

- [Netcat sources](https://github.com/openbsd/src/blob/master/usr.bin/nc/netcat.c)
- [Utilisation des sockets Internet](http://vidalc.chez.com/lf/socket.html)
- [Poll man page](https://linux.die.net/man/3/poll)
