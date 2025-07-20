// gcc parser.c -o parser.exe -lws2_32 -> linha para compilação correta

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <windows.h>
#include <winSock2.h>
#include <ws2tcpip.h>

// D N S  S E R V E R S (temporariamente vou deixar no fonte)
#define _GOOGLE_DNS_IP "8.8.8.8"
#define _CLOUDFARE_IP "1.1.1.1"

// D N S 
typedef struct {
    unsigned short id;
    unsigned char rd :1;
    unsigned char tc :1;
    unsigned char aa :1;
    unsigned char opcode :4;
    unsigned char qr :1;
    unsigned char rcode :4;
    unsigned char z :3;
    unsigned char ra: 1;
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


// O B J E C T  R E S U L T
typedef struct {
    bool success;
    char hostname[255];
    char ip[15];
} HostResult;


// LOCAL CACHE
int is_comment_or_empty(const char *line)
{
    while (*line == ' ' || *line == '\t') {
        ++line;
    }
    
    return (*line == '#' || *line == '\n' || *line == '\0');
}

void parse_hosts_line(const char *line, char *ip, char *hostname)
{
    if (is_comment_or_empty(line) == 0)
    {
        ip[0] = '\0';
        hostname[0] = '\0';
        return;
    }
    
    char temp[256];
    strncpy(temp, line, sizeof(temp));
    temp[sizeof(temp) - 1] = '\0';
    
    char *token = strtok(temp, "\t");
    if (token != NULL)
    {
        strncpy(ip, token, 15);
        ip[15] = '\0';
        
        token = strtok(NULL, " \t");
        if (token != NULL)
        {
            strncpy(hostname, token, 255);
            hostname[255] = '\0';
        }
        else 
        {
            hostname[0] = '\0';
        }
    }
    else
    {
        ip[0] = '\0';
        hostname[0] = '\0';
    }
}

HostResult* read_hosts_file(const char *domain)
{
    HostResult *result = malloc(sizeof(HostResult));
    memset(result, 0, sizeof(HostResult));
    result->success = false;
    
    const char *_WIN_SOURCE = "c:\\Windows\\System32\\drivers\\etc\\hosts";
    const char *_LINUX_SOURCE = "/etc/hosts";
    
    FILE *file;
    
    #if defined(_WIN32) || defined(_Win64)
        file = fopen(_WIN_SOURCE, "r");
    #elif defined(_linux_)
        file = fopen(_LINUX_SOURCE, "r");
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
        
        if (ip[0] != '\0' && hostname[0] != '\0' && strcmp(hostname, domain) == 0)
        {
            result->success = true;
            strncpy(result->ip, ip, 15);
            strncpy(result->hostname, hostname, 255);
            break;
        }
    }
    
    fclose(file);
    return result;
}

// D N S   R E S O L V E

// ajusta os caracteres do host para o formato adequado ao cabeçalho do protocolo DNS
void change_to_dns_name_format(unsigned char *dns, unsigned char *host)
{
    int lock = 0, i;
    strcat((char*)host, "."); // concatena a string aramzenada em HOST com "."
    //host = "example.com" => strcat((char*)host, ".") => *host < "example.com." 
    for (i = 0; i < strlen((char*)host); ++i) {
        if (host[i] == '.') 
        {
            *dns++ = i - lock;
            for (; lock < i; ++lock) {
                *dns++ = host[lock];
            }
            ++lock;
        }
    }
    
    *dns++ = '\0';
}


// inet_pton não tava pegando, famosa gambiarra (só aceita IPv4, melhorar para IPv6 futuramente)
int custom_inet_pton(const char *cp, void *addr)
{
    unsigned int octets[4] = {0};
    int count = 0;
    
    count = sscanf(cp, "%u.%u.%u.%u", &octets[0], &octets[1], &octets[2], &octets[3]);
    
    if (count != 4) return 0;
    
    for (int i = 0; i < 4; ++i) {
        if (octets[i] > 255) return 0;
    }
    
    unsigned int *addr_ptr = (unsigned int *)addr;
    *addr_ptr = (octets[0] << 24) | (octets[1] << 16) | (octets[2] << 8) | octets[3];
    
    return 1;
}


// CRAIA SOCKET DE REDE PARA CONECTAR NOS SERVIDORES DE DNS (ainda em dúvida se vai ser google ou cloudfare)
__declspec(dllexport) int create_udp_socket(unsigned short dns_ip)
{
    int rc, i;

    SOCKET sd;

    struct sockaddr_in cliAddr, dnsServAddr;
    struct hostent *lpHostEntry;
    struct in_addr addr;
    
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 1), &wsaData) != 0)
    {
        printf("Falha ao inicializar WinSock");
        return -1;
    }

    sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (sd < 0)
    {
        printf("Falha ao criar socket");
        WSACleanup();
        return -1;
    }

    cliAddr.sin_family = AF_INET;
    cliAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cliAddr.sin_port = htons(0);

    if (dns_ip == 0) 
    {
        if (custom_inet_pton(_GOOGLE_DNS_IP, &addr) != 1)
        {
            printf("Falha ao converter endereço IP do GOOGLE DNS\n");
            closesocket(sd);
            WSACleanup();
            return -1;
        }
    }
    else if (dns_ip == 1) 
    {
        if (custom_inet_pton(_CLOUDFARE_IP, &addr) != 1)
        {
            printf("Falha ao converter endereço IP do CLOUDFARE DNS\n");
            closesocket(sd);
            WSACleanup();
            return -1;
        }
    }
    else
    {
        printf("DNS selecionado não correspondente");
        closesocket(sd);
        WSACleanup();
        return -1;
    }

    lpHostEntry = gethostbyaddr((const char*)&addr, sizeof(addr), AF_INET);
    if (lpHostEntry == NULL)
    {
        printf("Falha ao obter informações do host: %d\n", WSAGetLastError());
        closesocket(sd);
        WSACleanup();
        return -1;
    }

    printf("DNS selecionado -> %s :: IP [%s]\n", lpHostEntry->h_name, inet_ntoa(addr));
    closesocket(sd);
    WSACleanup();

    return 0;
}