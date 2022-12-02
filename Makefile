main: server_skel.c client_skel.c
	gcc -o pa3_server server_skel.c
	gcc -o pa3_client client_skel.c

.PHONY: clean
clean:
	rm pa3_client pa3_server