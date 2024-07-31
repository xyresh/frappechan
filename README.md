<img src="https://github.com/xyresh/frappechan/blob/main/static/img/frappelogo.png" width=65px height=65px> 

# Frappechan 
 The kafeneio of chans

 

 Frappechan is a imageboard webapp built exclusively with C, and SQLite.
 The goal of this project was to have an extremely memory efficient backend, and frontend by minimizing javascript to just a few lines.

 you can compile and run Frappechan along with frappemod as such:  
 
 1.install dependencies: `libmicrohttpd-dev libsqlite3-dev libcjson-dev libssl-dev libncurses5-dev`  
 
 2.run `make`  
 
 3.run the server `./bin/frappe`  

 4.If you want to moderate your server(to archive/delete posts) you can run `./bin/frappemod`

 you can change any configurations you want within the `server.c` file, along with the frontend with your `index.html` file

## frappemod
 frappemod is a simple tool you can use to manage your instance's posts, by archiving and or deleting posts you may find violate your terms of service.
 it is built with ncurses, so you will need `libncurses5-dev`(on debian based systems) and of course all the other dependencies.


 building frappemod, is as simple as running `make mod`, and then running it.

 to use it, you may use <kbd>↑</kbd>, <kbd>↓</kbd>, then  <kbd>a</kbd> archive <kbd>d</kbd> delete and <kbd>r</kbd> to fetch the current posts.
