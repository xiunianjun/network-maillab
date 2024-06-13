#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

// #define TEST
#define MAX_SIZE 65535

char buf[MAX_SIZE+1];
const unsigned short port = 110; // POP3 server port
const char* host_name = "pop.163.com"; // TODO: Specify the mail server domain name
const char* user = "**********"; // TODO: Specify the user
const char* pass = "**********"; // TODO: Specify the password

void print_server_resp(int s_fd) {
    int r_size;
    if ((r_size = recv(s_fd, buf, MAX_SIZE, 0)) == -1)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }
    buf[r_size] = '\0'; // Do not forget the null terminator
    printf("%s", buf);
}

void recv_mail()
{
    char dest_ip[16];
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    char *send_str;
    struct sockaddr_in server_addr;

    // Get IP from domain name
    if ((host = gethostbyname(host_name)) == NULL)
    {
        herror("gethostbyname");
        exit(EXIT_FAILURE);
    }

    addr_list = (struct in_addr **) host->h_addr_list;
    while (addr_list[i] != NULL)
        ++i;
    strcpy(dest_ip, inet_ntoa(*addr_list[i-1]));

    // TODO: Create a socket,return the file descriptor to s_fd, and establish a TCP connection to the POP3 server
    s_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_fd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(dest_ip);
    bzero(server_addr.sin_zero, 8);

    if (connect(s_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect failed");
        close(s_fd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to the mail server %s:%d.\n", dest_ip, port);

    // Print welcome message
    print_server_resp(s_fd);

    // TODO: Send user and password and print server response
    send_str = "user %s\r\n";
    sprintf(buf, send_str, user);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    print_server_resp(s_fd);

    send_str = "pass %s\r\n";
    sprintf(buf, send_str, pass);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    print_server_resp(s_fd);

    // TODO: Send STAT command and print server response
    send_str = "stat\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);
    print_server_resp(s_fd);

    // TODO: Send LIST command and print server response
    send_str = "list\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);
    print_server_resp(s_fd);

    // TODO: Retrieve the first mail and print its content
    send_str = "retr 1\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);
    print_server_resp(s_fd);

    // TODO: Send QUIT command and print server response
    send_str = "QUIT\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);
    print_server_resp(s_fd);

    close(s_fd);
}

int main(int argc, char* argv[])
{
    recv_mail();
    exit(0);
}
