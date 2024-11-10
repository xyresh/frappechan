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

 5.You can run it as a systemd service, copy(and modify if necessary) the `frappe.service` file into the `/etc/systemd/system/` directory
 then simply `systemctl enable frappe` as root. I might change the directories to be fully unix compliant in the future, but as of now i find it is easier if it's in the home directory.

 If it fails to start MHD daemon, the problem may be one of the following:

 1.Something else is running on port 80 already (appache webserver or nginx) make sure these aren't running or change frappechans port in server.c

 2.insufficient privileges, either run as root, or run `setcap 'cap_net_bind_service=+ep' /bin/frappe` within the frappechan directory as root.

 It's also not a bad idea to run `ufw allow 80` as root

 you can change any configurations you want within the `server.c` file, along with the frontend with your `index.html` file

## frappemod
 frappemod is a simple tool you can use to manage your instance's posts, by archiving and or deleting posts you may find violate your terms of service.
 it is built with ncurses, so you will need `libncurses5-dev`(on debian based systems) and of course all the other dependencies.

![Screenshot 2024-11-08 at 9 55 15 PM](https://github.com/user-attachments/assets/082c6381-b679-48ae-a292-f7a123e75cef)




 building frappemod, is as simple as running `make mod`, and then running it.

## Running on other systems
  I have tested frappechan on debian GNU/Linux, it should run on linux distributions which have gcc.
  I have also been able to run in on macOS natively, and it also seems to work on openBSD.
  you should check the library equivalents for your system as i have provided the debian names of these packages.

 to use it, you may use <kbd>↑</kbd>, <kbd>↓</kbd>, then  <kbd>a</kbd> archive <kbd>d</kbd> delete and <kbd>r</kbd> to fetch the current posts.
