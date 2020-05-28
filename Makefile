my_server: my_server
		gcc -I"/home/pi/include" -L"/home/pi/lib" my_server.c -o my_server -lmicrohttpd 