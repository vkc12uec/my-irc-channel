all:
	g++ -g server.cpp -o myhttpdplus
	gcc -g selectserver.c -o myhttpd
	#gcc -g client.c -o client -lpthread -lrt
	#gcc -g client2.c -o client2 -lpthread
	#gcc selectserver.c -o myhttpd

run:
	./myhttpd
	

run1:
	./myhttpdplus

run11:
	myhttpd 1.1

run112222:
	myhttpd 1.1 2222

clean:
	rm myhttpd
	rm client

stats:
	perl parse.pl $@
