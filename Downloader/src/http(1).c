#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include "http.h"

#define BUF_SIZE 1024

Buffer* buffer_maker(void){   
	/*makes a buffer for the downloader to put data into*/
	
	Buffer* buffer = malloc(sizeof(Buffer));   
	buffer->data = malloc(BUF_SIZE);
	buffer->length = 0;    
	return buffer;
}

char* sort_query(char *host, char *page){
	/* 
	 * Turns the template query into one filled out with the
	 * relevant host and page names
	 * */
	char* template = "GET /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: getter\r\n\r\n";
	int leng = strlen(template) + strlen(host) + strlen(page);
	char* query = malloc(leng);
	snprintf(query, leng, template, page, host); //replaces the variable placeholders in the string with the proper values
	return query;
}


Buffer* http_query(char *host, char *page, int port) {
	struct addrinfo addrinfo, *servinfo;
	char* query = sort_query(host, page);   //gets a query for send()
    Buffer* buffer = buffer_maker();
    
    memset(&addrinfo, 0, sizeof addrinfo); 
	addrinfo.ai_family = AF_INET;    //ipv4
	addrinfo.ai_socktype = SOCK_STREAM; //tcp
	
    char strPort[6];
    sprintf(strPort, "%d", port); //convert the port to a string
   
    getaddrinfo(host, strPort, &addrinfo, &servinfo);
	
	int sock = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
	//bind(sock, servinfo->ai_addr, servinfo->ai_addrlen);
	connect(sock, servinfo->ai_addr, servinfo->ai_addrlen);
	send(sock, query, strlen(query), 0);
	
	int incoming_bytes;                     //how many bytes are to be downloaded
	while((incoming_bytes = recv(sock, buffer->data + buffer->length, BUF_SIZE, 0)) > 0){   //while there is more to download, put the data to be 
																							//downloaded into the buffer AFTER the data that has already been downloaded
		buffer->length += incoming_bytes;    //increase the buffer to facilitate more data
		//printf("%d\n", incoming_bytes);    //how many bytes are to be downloaded
		buffer->data = realloc(buffer->data, buffer->length + BUF_SIZE); //reallocate more memory for the new increased size
	}
	close(sock);
	free(query);
	freeaddrinfo(servinfo); //free the linked list
	
	return buffer;
}

// split http content from the response string
char* http_get_content(Buffer *response) {

    char* header_end = strstr(response->data, "\r\n\r\n");

    if (header_end) {
        return header_end + 4;
    }
    else {
        return response->data;
    }
}


Buffer *http_url(const char *url) {
    char host[BUF_SIZE];
    strncpy(host, url, BUF_SIZE);

    char *page = strstr(host, "/");
    if (page) {
        page[0] = '\0';

        ++page;
        return http_query(host, page, 80);
    }
    else {

        fprintf(stderr, "could not split url into host/page %s\n", url);
        return NULL;
    }
}

