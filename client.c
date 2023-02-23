#include <stdio.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>

#define MAX_REQ_SIZE 3000
#define MAX_HOST_SIZE 150
#define BUFF_SIZE 6000

typedef enum { FALSE = 0, TRUE } boolean;

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


// function prints usage message and exits

void printUsage()
{
    printf("Usage: client [-p n <text>] [-r n < pr1=value1 pr2=value2 …>] <URL>\n");
    exit(1);
}

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


// function gets port number, returns TRUE if valid

boolean isValidPort(int port)
{
    if(port<0 || port>65536)
        return FALSE;
    
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


// function string, returns TRUE if its a number

boolean isNumber(char* str)
{
    for(int i=0; i<strlen(str); i++)
    {
        if(str[i]<'0' || str[i]>'9')
            return FALSE;
    }
    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


//  function gets url , fills up host, filepath & port variables

char* parseURL(char* url, char* host, int* port)
{
    int k=0;    // port index
    int l=0;    // filepath index
    int j=0;    // host index
    
    char check_port[10] ;
    
    char* filepath = (char*) malloc((strlen(url)-sizeof(host))*sizeof(char));
    if(filepath==NULL)
    {
        perror("malloc\n");
        exit(1);
    }
    
    //  go through URL
    for(int i=7; i<strlen(url); i++)
    {
        
        //  find port number
        if(url[i]==':')
        {
            i++;
            
            //  keep going until filepath
            while(url[i]!='\0')
            {
                if(url[i]=='/')
                    break;
                 
                //  get port number   
                check_port[k]= url[i];
                k++;
                
                i++;
            }
            check_port[k] = '\0';
            
            // check if port is valid
            if(isNumber(check_port)==TRUE)
            {
                *port = atoi(check_port);
                if(isValidPort(*port)==FALSE)
                {
                    // invalid port
                    printUsage();
                }
            }
            else
            {
                printUsage();
            }
        }
        
        // find filepath
        if(url[i]=='/')
        {
            // keep going through string
            while(url[i]!='\0')
            {
                // get filepath
                filepath[l] = url[i];
                i++;
                l++;
                
            }
            filepath[l] = '\0';
        }
        
        // get host
        host[j] = url[i];
        j++;
    }
    
    host[j]='\0';

    return filepath;

}

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


// function takes main arguments and builds a HTTP request

void args_to_request(int argc , char* argv[], char* http_request, char* host, int* port)
{

    int arg_count = 1;
    
    char* filepath = NULL;
    char* content = NULL;
    char* request = NULL; 
    
    int np_index = -1;      // -p's number index
    int nr_index = -1;      // -r's number index
    int np = -1;            // -p's number
    int nr = -1;            // -r's number
    
    boolean found_r = FALSE;
    boolean found_p = FALSE;
    boolean found_url = FALSE;
    
    
    // go through arguments
    for(int i=1 ; i<argc; i++)
    {
        // look for URL
        char* check_url = NULL;
        check_url = strstr(argv[i], "http://");
        
        // found URL
        if(check_url!=NULL)
        {
            if(found_url == TRUE || strcmp(check_url, argv[i])!=0)
                printUsage();
            
            // parse URL
            filepath = parseURL(check_url, host, port);
            found_url = TRUE;
            arg_count++;
        }
        
        
        // found -
        if(argv[i][0]=='-')
        {
            // check illegal parameters
            if(argv[i][1]!='p' && argv[i][1]!='r')
                printUsage();
            
            // found -p
            if(argv[i][1]=='p')
            {
                if(found_p == TRUE)
                    printUsage();
                    
                found_p = TRUE;
                arg_count++;
                np_index = i+1;
                
            }
            
            // found -r
            if(argv[i][1]=='r')
            {
                if(found_r == TRUE)
                    printUsage();
                    
                found_r = TRUE;
                arg_count++;
                nr_index = i+1;
            }
        }
        
        
        // find -p's number
        if(np_index!=-1 && i==np_index)
        {
            // check that it is a valid number
            if(isNumber(argv[i])==TRUE && strcmp(argv[i], "0")!=0)
            {
                np = atoi(argv[i]);    
                np_index = i;
                arg_count++;
            }
            else    // invalid number
                printUsage();
        }
        
        
        // find -p content
        if(np!=-1 && np_index!=-1 && i==np_index+1)
        {
            
            // if content length is bigger than -p's number
            if(np>strlen(argv[i]))
                printUsage();
            
            // if content is smaller than -p's number
            if(np<strlen(argv[i]))
            {
            
                content = (char*) malloc(np*sizeof(char));
                
                // copy only 'np' chars
                for(int k=0; k<np; k++)
                    content[k] = argv[i][k];
                
                content[np] = '\0';
            }
            
            // contents length is same size as -p's number    
            else
            {
                int size = strlen(argv[i]) +1 ;
                content = (char*) malloc(size*sizeof(char));
                strcpy(content, argv[i]);
            }
                
            arg_count++;
        }
        
        
        // find -r number
        if(nr_index!=-1 && i==nr_index)
        {
            // check it's a valid number
            if(isNumber(argv[i])==TRUE)
            {
                request = (char*) malloc(MAX_REQ_SIZE * sizeof(char) );
                if(request==NULL)
                {
                    perror("malloc\n");
                    exit(1);
                }
                nr = atoi(argv[i]);    
                nr_index = i;
                if(nr!=0)
                    strcpy(request, "?");
                arg_count++;
                
            }
            else    // -r's number isn't valid
                printUsage();
        }
        
        
        // find -r arguments
        if(nr!=-1 && nr_index!=-1 && i>nr_index && i<=nr_index+nr)
        {
            arg_count++;
            
            // go through argument
            for(int q=1; q<strlen(argv[i]); q++)
            {
                // not right format : <arg = value>
                if(q==strlen(argv[i])-1)
                    printUsage();
                    
                if(argv[i][q]=='=')
                    break;
            }
            if(i!=nr_index+1)
                strcat(request, "&");
            strcat(request, argv[i]);
        }
    }
    
    
    if(nr!=0 && argc != arg_count)
        printUsage();
    
    
    // start building http request
    
    if(np_index!=-1)
        strcpy(http_request, "POST ");
    else strcpy(http_request, "GET ");
    
    if(filepath!=NULL && strlen(filepath)!=0)
    {
        strcat(http_request, filepath);
    }    
    if(request!=NULL && strlen(request)!=0)
        strcat(http_request, request);
    

    if( (filepath==NULL && request==NULL) || 
        (filepath==NULL && request!=NULL && strlen(request)==0)  ||
        (filepath!=NULL && strlen(filepath)==0 && request==NULL ) ||
        (filepath!=NULL && request!=NULL && strlen(filepath)==0 && strlen(request)!=0))
            {
                strcat(http_request, "/");
            }        
    strcat(http_request, " HTTP/1.0\r\nHost: ");
    strcat(http_request, host);
    if(np_index!=-1)
    {
        strcat(http_request, "\r\nContent-Length: ");
        char num[100];
        sprintf(num, "%ld", strlen(content));
        strcat(http_request, num);
    }
    strcat(http_request, "\r\n\r\n");
    
    if(np_index!=-1)
        strcat(http_request, content);
    
    free(filepath);
    free(request);
    free(content);
  
}

////////////////////////////////////////////////////////////////////////////////////////////////
//..............................................................................................
////////////////////////////////////////////////////////////////////////////////////////////////


// program takes argv and makes it into a HTTP requeset,
// sends it and waits for reply from server

int main( int argc, char *argv[] )
{

    char* host = NULL;
    char http_request[MAX_REQ_SIZE] = "/";
    int port = 80;
    
    
    if(argc>1)
    {
        host = (char*) malloc(MAX_HOST_SIZE*sizeof(char));
        if(host==NULL)
        {
            perror("malloc");    
            exit(1);
        }
        
        // build HTTP request
        args_to_request(argc, argv ,http_request, host, &port);
        
        printf("________________________\n");
        printf("HTTP request =\n%s\nLEN = %ld\n", http_request, strlen(http_request));
        printf("________________________\n");
        
        // initialize a TCP socket
        int fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if ( fd < 0 )
        {   
            free(host);
            perror("socket error\n");
            exit(1);
        }    
        
        // initialize sock address struct 
        struct sockaddr_in server;
        server.sin_family = AF_INET;
        server.sin_port = htons(port);
        struct hostent *hp;
        
        // DNS request, getting server's IP
        hp = gethostbyname(host);
        if (hp == NULL) 
        { 
            while(h_errno==TRY_AGAIN)
                hp = gethostbyname(host);
            if(hp==NULL)
            {
                free(host);
                herror("gethostbyname"); 
                exit(1);
                
            }
        }
        // set server's IP
        server.sin_addr.s_addr = ((struct in_addr*)hp->h_addr)->s_addr;
        
        // connect to server
        if(connect(fd, (struct sockaddr*)&server, sizeof(server) ) < 0 )
        {
            free(host);
            perror("connect");
            exit(1);
        }
        
        int chars_to_write = strlen(http_request);
        int written = 0 ;
        int sent = 0;
        
        int read_so_far = 0;
        int nread = 0;
        unsigned char buff[BUFF_SIZE];
        
        
        //  send HTTP packet
        while(TRUE)
        {
            sent = write(fd, http_request, chars_to_write-written);
            if(sent<0)
            {
                free(host);
                perror("write");
                exit(1);
                
            }
            written += sent;
            
            if(written == chars_to_write)
                break;
        }
            
        
        //  recieve response from server
        while(TRUE)
        {
            nread = read(fd, buff, sizeof(buff));
            
            if(nread<0)
            {
                free(host);
                perror("read");
                exit(1);
            }
            if(write(1, buff, nread)<0)
            {
                free(host);
                perror("write");
                exit(1);
            }
            read_so_far += nread;
            
            if(nread==0)
                break;
        }
        
        printf("\nTotal received response bytes: %d\n", read_so_far);
        
        free(host);
        close(fd);
        
        return 0;
        
    }
    
    else
        printUsage();
    
}






