#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>

/**
 * Project 1 starter code
 * All parts needed to be changed/added are marked with TODO
 */

#define BUFFER_SIZE 1024
#define DEFAULT_SERVER_PORT 8081
#define DEFAULT_REMOTE_HOST "131.179.176.34"
#define DEFAULT_REMOTE_PORT 5001

struct server_app {
    // Parameters of the server
    // Local port of HTTP server
    uint16_t server_port;

    // Remote host and port of remote proxy
    char *remote_host;
    uint16_t remote_port;
};

// The following function is implemented for you and doesn't need
// to be change
void parse_args(int argc, char *argv[], struct server_app *app);

// The following functions need to be updated
void handle_request(struct server_app *app, int client_socket);
void serve_local_file(int client_socket, const char *path);
void proxy_remote_file(struct server_app *app, int client_socket, const char *path);

//helper function
void decode_filepath_encoding(char *str);

// The main function is provided and no change is needed
int main(int argc, char *argv[])
{
    struct server_app app;
    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len;
    int ret;

    parse_args(argc, argv, &app);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(app.server_port);

    // The following allows the program to immediately bind to the port in case
    // previous run exits recently
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 10) == -1) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d\n", app.server_port);

    while (1) {
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket == -1) {
            perror("accept failed");
            continue;
        }
        
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        handle_request(&app, client_socket);
        close(client_socket);
    }

    close(server_socket);
    return 0;
}

void parse_args(int argc, char *argv[], struct server_app *app)
{
    int opt;

    app->server_port = DEFAULT_SERVER_PORT;
    app->remote_host = NULL;
    app->remote_port = DEFAULT_REMOTE_PORT;

    while ((opt = getopt(argc, argv, "b:r:p:")) != -1) {
        switch (opt) {
        case 'b':
            app->server_port = atoi(optarg);
            break;
        case 'r':
            app->remote_host = strdup(optarg);
            break;
        case 'p':
            app->remote_port = atoi(optarg);
            break;
        default: /* Unrecognized parameter or "-?" */
            fprintf(stderr, "Usage: server [-b local_port] [-r remote_host] [-p remote_port]\n");
            exit(-1);
            break;
        }
    }

    if (app->remote_host == NULL) {
        app->remote_host = strdup(DEFAULT_REMOTE_HOST);
    }
}

void handle_request(struct server_app *app, int client_socket) {
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    // Read the request from HTTP client
    // Note: This code is not ideal in the real world because it
    // assumes that the request header is small enough and can be read
    // once as a whole.
    // However, the current version suffices for our testing.
    bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_read <= 0) {
        return;  // Connection closed or error
    }

    buffer[bytes_read] = '\0';
    // copy buffer to a new string
    char *request = malloc(strlen(buffer) + 1);
    strcpy(request, buffer);

    //print request
    //printf("%s", request);

    // TODO: Parse the header and extract essential fields, e.g. file name
    // Hint: if the requested path is "/" (root), default to index.html
    char *file_name = "index.html";

    //file name can have alphabet, periods, spaces, percentages
    char *tok = strtok(request, " ");

    tok = strtok(NULL, "/");

    // url formatted as url HTTP    
    char *url = tok;
    // chops off HTTP the end
    url[strlen(url)-5] = '\0';

    if(url[0] != '\0'){
        file_name = url;
    } 
    //printf("file_name: %s\n", file_name);

    decode_filepath_encoding(file_name);

    // TODO: Implement proxy and call the function under condition
    // specified in the spec
    // if (need_proxy(...)) {
    //    proxy_remote_file(app, client_socket, file_name);
    // } else {
    char *ext = strrchr(file_name, '.');
    if(ext != NULL && strcmp(ext, ".ts") == 0){
        proxy_remote_file(app, client_socket, file_name);
    }
    else {
        serve_local_file(client_socket, file_name);
    }
    //}
}

void decode_filepath_encoding(char *str){
    int len = strlen(str);
    for(int i=0; i < len; i++){
        if ((str[i] == '%' && i < len-2) && str[i+1] == '2'){
            if (str[i+2] == '0'){   //space
                str[i] = ' ';
            } 
            else if (str[i+2] == '5'){
                str[i] = '%';
            }
            memmove(&str[i+1], &str[i+3], len-i-2);
            len -= 2;
        }
    }
}

void serve_local_file(int client_socket, const char *path) {
    // TODO: Properly implement serving of local files
    // The following code returns a dummy response for all requests
    // but it should give you a rough idea about what a proper response looks like
    // What you need to do 
    // (when the requested file exists):
    // * Open the requested file
    // * Build proper response headers (see details in the spec), and send them
    // * Also send file content
    // (When the requested file does not exist):
    // * Generate a correct response
    char *response = (char*)malloc(1000);
    char *ext = strrchr(path, '.');     //finds last . because there can be periods in the path
    if(ext == NULL){
        FILE*  fp = fopen(path, "rb"); 
    
        // checking if the file exist or not 
        if (fp == NULL) { 
            response = "HTTP/1.0 404 Not Found\r\n";
            send(client_socket, response, strlen(response), 0);
        } 
        
        else {
            fseek(fp, 0L, SEEK_END); 
            long int file_size = ftell(fp); 
            char *file_content = (char*)malloc(file_size);
            fseek(fp, 0, SEEK_SET);
            fread(file_content, file_size, 1, fp);
            fclose(fp);
            snprintf(response, 100 + file_size, "HTTP/1.0 200 OK\r\nContent-Type: application/octet-stream\r\nContent-Length: %ld\r\n\r\n", file_size);
            send(client_socket, response, strlen(response), 0);
            send(client_socket, file_content, file_size, 0);
            free(file_content);
        }
        // response = "HTTP/1.0 200 OK\r\n"
        //                 "Content-Type: application/octet-stream\r\n"
        //                 "Content-Length: 15\r\n"
        //                 "\r\n"
        //                 "Sample response";
    } 

    else if(strcmp(ext, ".html") == 0){
        FILE*  fp = fopen(path, "r"); 
    
        // checking if the file exist or not 
        if (fp == NULL) { 
            response = "HTTP/1.0 404 Not Found\r\n";
            send(client_socket, response, strlen(response), 0);
        } 

        else {
            fseek(fp, 0L, SEEK_END); 
            long int file_size = ftell(fp); 
            char *file_content = (char*)malloc(file_size + 1);
            fseek(fp, 0, SEEK_SET);
            fread(file_content, file_size, 1, fp);
            file_content[file_size] = '\0';
            fclose(fp);
            snprintf(response, 100 + file_size, "HTTP/1.0 200 OK\r\nContent-Type: text/html; charset=UTF-8\r\nContent-Length: %ld\r\n\r\n", file_size);
            send(client_socket, response, strlen(response), 0);
            send(client_socket, file_content, file_size, 0);
            free(file_content);      
        }
    }
    else if(strcmp(ext, ".txt")  == 0){
        FILE*  fp = fopen(path, "r"); 
    
        // checking if the file exist or not 
        if (fp == NULL) { 
            response = "HTTP/1.0 404 Not Found\r\n";
            send(client_socket, response, strlen(response), 0);
        } 

        else {
            fseek(fp, 0L, SEEK_END); 
            long int file_size = ftell(fp); 
            char *file_content = (char*)malloc(file_size + 1);
            fseek(fp, 0, SEEK_SET);
            fread(file_content, file_size, 1, fp);
            file_content[file_size] = '\0';
            fclose(fp);
            snprintf(response, 100 + file_size, "HTTP/1.0 200 OK\r\nContent-Type: text/plain; charset=UTF-8\r\nContent-Length: %ld\r\nX-Content-Type-Options: nosniff\r\n\r\n", file_size);
            send(client_socket, response, strlen(response), 0);
            send(client_socket, file_content, file_size, 0); 
            free(file_content);     
        }
    }

    else if(strcmp(ext, ".jpg") == 0){
        FILE*  fp = fopen(path, "rb"); 
    
        if (fp == NULL) { 
            response = "HTTP/1.0 404 Not Found\r\n";
            send(client_socket, response, strlen(response), 0);
        } 

        else{
            fseek(fp, 0L, SEEK_END); 
            long int file_size = ftell(fp); 
            char *file_content = (char*)malloc(file_size);
            fseek(fp, 0, SEEK_SET);
            fread(file_content, file_size, 1, fp);
            file_content[file_size] = '\0';
            fclose(fp);
            snprintf(response,  100 + file_size, "HTTP/1.0 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: %ld\r\n\r\n", file_size); 
            send(client_socket, response, strlen(response), 0);
            send(client_socket, file_content, file_size, 0);     
            free(file_content);
        }
    }   

    else {
        response = "HTTP/1.0 400 Bad Request\r\n";
        send(client_socket, response, strlen(response), 0);
    }

}

void proxy_remote_file(struct server_app *app, int client_socket, const char *request) {
    // TODO: Implement proxy request and replace the following code
    // What's needed:
    // * Connect to remote server (app->remote_server/app->remote_port)
    // * Forward the original request to the remote server
    // * Pass the response from remote server back
    // Bonus:
    // * When connection to the remote server fail, properly generate
    // HTTP 502 "Bad Gateway" response

    printf("host: %s", app->remote_host);
    printf("port: %hu", app->remote_port);

    struct sockaddr_in remote_addr;
    remote_addr.sin_family = AF_INET;
    inet_pton(AF_INET, app->remote_host, &remote_addr.sin_addr);
    remote_addr.sin_port = htons(app->remote_port); 

    int server_socket, bytes_recv;
    char buffer[BUFFER_SIZE];

    if (server_socket = socket(AF_INET, SOCK_STREAM, 0) == -1) {
        perror("server socket failed");
        exit(EXIT_FAILURE);
    }

    if (connect(server_socket, (struct sockaddr *)&remote_addr, sizeof(remote_addr)) == -1) {
        close(server_socket);
        char response[] = "HTTP/1.0 502 Bad Gateway\r\n\r\n";
        send(client_socket, response, strlen(response), 0);
        return;
    }

    if (send(server_socket, request, strlen(request), 0) == -1) {
        close(server_socket);
        perror("send to server failed");
        exit(EXIT_FAILURE);
    }

    //receive data in buffer and pass to client
    

    close(server_socket);

    //char response[] = "HTTP/1.0 501 Not Implemented\r\n\r\n";
    //send(client_socket, response, strlen(response), 0);
}