all:
	cc -o bin/frappe src/server.c src/database.c -lmicrohttpd -lsqlite3 -lcjson -lssl -lcrypto 		-I/usr/include/cjson/ -L/usr/include/cjson/
	cc -o bin/frappemod src/frappemod.c -lsqlite3 -lncursesw

server:
	cc -o bin/frappe src/server.c src/database.c -lmicrohttpd -lsqlite3 -lcjson -lssl -lcrypto 		-I/usr/include/cjson/ -L/usr/include/cjson/

mod:
	cc -o bin/frappemod src/frappemod.c -lsqlite3 -lncursesw