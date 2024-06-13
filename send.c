#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <getopt.h>
#include "base64_utils.h"

// If defined TEST, the sender is hitsz-lab.com; else is 163 mail.
#define TEST
#define MAX_SIZE 4095

char buf[MAX_SIZE+1];
const unsigned short port = 25; // SMTP server port
#ifdef TEST
const char* host_name = "hitsz-lab.com"; // TODO: Specify the mail server domain name
const char* user = ""; // TODO: Specify the user
const char* pass = ""; // TODO: Specify the password
const char* from = "xiunian@hitsz-lab.com"; // TODO: Specify the mail address of the sender
#else
const char* host_name = "smtp.163.com"; // TODO: Specify the mail server domain name
const char* user = "**********"; // TODO: Specify the user
const char* pass = "**********"; // TODO: Specify the password
const char* from = "**********"; // TODO: Specify the mail address of the sender
#endif

// usage: get resp from the peer and do some processing
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

// receiver: mail address of the recipient
// subject: mail subject
// msg: content of mail body or path to the file containing mail body
// att_path: path to the attachment
void send_mail(const char* receiver, const char* subject, const char* msg, const char* att_path)
{
    const char* end_msg = "\r\n.\r\n";
    char dest_ip[16]; // Mail server IP address
    int s_fd; // socket file descriptor
    struct hostent *host;
    struct in_addr **addr_list;
    int i = 0;
    char *send_str;
    struct sockaddr_in server_addr;
    size_t bytes_read, bytes_sent, sent;

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

    // TODO: Create a socket, return the file descriptor to s_fd, and establish a TCP connection to the mail server
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

    // Send EHLO command and print server response
    sprintf(buf, "EHLO %s\r\n", host_name);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);

    // TODO: Print server response to EHLO command
    print_server_resp(s_fd);

    // TODO: Authentication. Server response should be printed out.
#ifndef TEST    // localhost has no need to do an authentication
    const char* LOGIN = "AUTH login\r\n";
    send(s_fd, LOGIN, strlen(LOGIN), 0);
    printf("%s", LOGIN);
    print_server_resp(s_fd);

    char *res_str = encode_str(user);
    send(s_fd, res_str, strlen(res_str), 0);
    printf("%s", res_str);
    free(res_str);
    print_server_resp(s_fd);

    res_str = encode_str(pass);
    send(s_fd, res_str, strlen(res_str), 0);
    printf("%s", res_str);
    free(res_str);
    print_server_resp(s_fd);
#endif

    // TODO: Send MAIL FROM command and print server response
    sprintf(buf, "MAIL FROM:<%s>\r\n", from);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    print_server_resp(s_fd);

    // TODO: Send RCPT TO command and print server response
    sprintf(buf, "RCPT TO:<%s>\r\n", receiver);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    print_server_resp(s_fd);

    // TODO: Send DATA command and print server response
    send_str = "data\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);
    print_server_resp(s_fd);

    // TODO: Send message data
    // send subject
    sprintf(buf, "subject:%s\r\n", subject);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    // send from
    sprintf(buf, "from:%s\r\n", from);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    // send to
    sprintf(buf, "to:%s\r\n", receiver);
    send(s_fd, buf, strlen(buf), 0);
    printf("%s", buf);
    // send MIME
    send_str = "MIME-Version:1.0\r\nContent-Type: multipart/mixed; boundary=\"XYZ\"\r\n\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);

    // send email content
    if (msg) {
        send_str = "--XYZ\r\nContent-Type:text/plain\r\n\r\n";
        send(s_fd, send_str, strlen(send_str), 0);
        printf("%s", send_str);

        FILE *fp = fopen(msg, "r");
        if (!fp)
        {
            send(s_fd, msg, strlen(msg), 0);
            printf("%s", msg);
        } else {
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);
            char *content = (char *)malloc(sizeof(char) * (MAX_SIZE + 1));
            fread(content, 1, file_size, fp);

            fclose(fp);
            
            send(s_fd, content, strlen(content), 0);
            free(content);
        }
        send_str = "\r\n";
        send(s_fd, send_str, strlen(send_str), 0);
        printf("%s", send_str);
    }
    
    // send attached files
    if (att_path) {
        FILE *fp = fopen(att_path, "r");
        if (!fp)
        {
            perror("file not found");
            close(s_fd);
            exit(EXIT_FAILURE);
        }

        sprintf(buf, "--XYZ\r\nContent-Type:application/octet-stream\r\nContent-Disposition:attachment; name=%s; filename=%s\r\nContent-Transfer-Encoding: base64\r\n\r\n", att_path, att_path);
        send(s_fd, buf, strlen(buf), 0);
        printf("%s", buf);

        FILE *fp_temp = fopen("temp", "w");
        encode_file(fp, fp_temp);
        fclose(fp);
        fclose(fp_temp);

        fp_temp = fopen("temp", "r");
        fseek(fp_temp, 0, SEEK_END);
        long file_size = ftell(fp_temp);
        fseek(fp_temp, 0, SEEK_SET);
        char *content = (char *)malloc(sizeof(char) * (MAX_SIZE + 1));
        fread(content, 1, file_size, fp_temp);
        fclose(fp_temp);
        send(s_fd, content, strlen(content), 0);
        remove("temp");        
        free(content);
    }

    send_str = "--XYZ--\r\n";
    send(s_fd, send_str, strlen(send_str), 0);
    printf("%s", send_str);

    // TODO: Message ends with a single period
    send(s_fd, end_msg, strlen(end_msg), 0);
    printf("%s", end_msg);
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
    int opt;
    char* s_arg = NULL;
    char* m_arg = NULL;
    char* a_arg = NULL;
    char* recipient = NULL;
    const char* optstring = ":s:m:a:";
    while ((opt = getopt(argc, argv, optstring)) != -1)
    {
        switch (opt)
        {
        case 's':
            s_arg = optarg;
            break;
        case 'm':
            m_arg = optarg;
            break;
        case 'a':
            a_arg = optarg;
            break;
        case ':':
            fprintf(stderr, "Option %c needs an argument.\n", optopt);
            exit(EXIT_FAILURE);
        case '?':
            fprintf(stderr, "Unknown option: %c.\n", optopt);
            exit(EXIT_FAILURE);
        default:
            fprintf(stderr, "Unknown error.\n");
            exit(EXIT_FAILURE);
        }
    }

    if (optind == argc)
    {
        fprintf(stderr, "Recipient not specified.\n");
        exit(EXIT_FAILURE);
    }
    else if (optind < argc - 1)
    {
        fprintf(stderr, "Too many arguments.\n");
        exit(EXIT_FAILURE);
    }
    else
    {
        recipient = argv[optind];
        send_mail(recipient, s_arg, m_arg, a_arg);
        exit(0);
    }
}
