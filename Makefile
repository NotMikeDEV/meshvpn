CC=gcc

install: meshvpn
	cp meshvpn /sbin/meshvpn
meshvpn: clean
	${CC} -g -o meshvpn meshvpn.c node.c crypto.c network.c router.c -ldl -lcrypto -lssl
debian:
	apt install 
clean:
	rm -f meshvpn
