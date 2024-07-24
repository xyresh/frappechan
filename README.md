<img src="https://github.com/xyresh/frappechan/blob/main/static/img/frappelogo.png" width=65px height=65px> 

# Frappechan 
 The kafeneio of chans

 

 Frappechan is a imageboard webapp built exclusively with C, and SQLite.
 The goal of this project was to have an extremely memory efficient backend, and frontend by minimizing javascript to just a few lines.

 you can compile and run Frappechan as such:  
 
 1.install dependencies: `libmicrohttpd-dev libsqlite3-dev libcjson-dev libssl-dev`  
 
 2.run `make`  
 
 3.run the server `./bin/frappe`  
 

 you can change any configurations you want within the `server.c` file, along with the frontend with your `index.html` file
