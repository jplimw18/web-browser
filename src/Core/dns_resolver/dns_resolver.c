#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

typedef struct {
    unsigned short id;
    unsigned char rd :1;
    unsigned char tc :1;
    unsigned char aa :1;
    unsigned char opcode :4;
    unsigned char qr :1;
    unsigned char rcode :4;
    unsigned char z :3;
    unsigned char ra :1;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
} DNS_HEADER;

typedef struct {
    unsigned short qtype;
    unsigned short qclass;
} QUESTION;

typedef struct {
    unsigned short type;
    unsigned short class;
    unsigned int ttl;
    unsigned short data_len;
} R_DATA;

typedef struct {
    unsigned char *name;
    R_DATA *resource;
    unsigned char *rdata;
} RES_RECORD;


typedef struct {
    bool success;
    unsigned char hostname[255];
    unsigned char ip[15];
} HOST_RESULT;


bool is_comment_or_empty(const char *line)
{
    while (*line == '' || *line == '\t') {
        ++line;
    }

    return (*line == '#' || *line == '\n' || *line == '\0');
}

void parse_hosts_line(const char *line, char *ip, char *hostname) 
{
    if (is_comment_or_empty(line))
    {
        ip[0] = '\0';
        hostname[0] = '\0';
        return; 
    }

    char temp[256];
    strncpy(temp, line, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';

    char *token = strtok(temp, '\t');
    if (token != NULL)
    {
        strncpy(hostname, token, 255);
        ip[15] = '\0';

        token = strtok(NULL, " \t");
        if (token != NULL)
        {
            strncpy(hostname, token, 255);
            hostname[255] = '\0';
        }
        else 
            hostname[0] = '\0';
    }
    else
    {
        ip[0] = '\0';
        hostname[0] = '\0';
    }
}

HOST_RESULT* read_hosts_file(const char *domain)
{
    HOST_RESULT *result = malloc(sizeof(result));
    memset(result, 0, sizeof(HOST_RESULT));
    result->success = false;
    result->ip = {0};
    result->hostname = {0};

    const char *WIN_SOURCE = "c:\\Windows\\System32\\drivers\\etc\\hosts";
    const char *LINUX_SOURCE = "/etc/hosts";

    FILE *file;

    #if defined(_WIN32) || defined(_WIN64)
        file = fopen(WIN_SOURCE, "r");
    #elif defined(__linux__)
        file = fopen(LINUX_SOURCE, "r");
    #else
        file = NULL;
    #endif


    if (!file)
    {
        return result;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char ip[16] = {0};
        char hostname[256] = {0};

        parse_hosts_line(line, ip, hostname);

        if (ip[0] != '\0' && hostname[0] != '\0' && strcmp(hostname, domain))
        {
            result->success = true;
            strncpy(result->ip, ip, sizeof(ip) - 1);
            strncpy(result->hostname, hostname, sizeof(hostname) - 1);
            break;
        }
    }

    fclose(file);
    return result;
}