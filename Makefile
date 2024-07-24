all:
	cc -o bin/frappe src/server.c src/database.c -lmicrohttpd -lsqlite3 -lcjson -I/usr/include/cjson/ -L/usr/include/cjson/