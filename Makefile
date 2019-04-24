install: meshvpn
	cp meshvpn /sbin/meshvpn
meshvpn: clean
	g++ -O3 -o meshvpn *.cpp -ldl -lcrypto -lssl -std=c++11
clean:
	rm -f meshvpn
