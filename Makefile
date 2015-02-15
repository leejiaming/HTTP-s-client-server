all: http_server http_client https_server

http_server: http_server.c
	gcc -o http_server http_server.c

https_server: https_server.c
	gcc -o https_server https_server.c connect_tls.c -lpolarssl  

http_client: http_client.c
	gcc -o http_client http_client.c connect_tls.c -lpolarssl


.PHONY : clean

clean:
	rm http_server https_server http_client
