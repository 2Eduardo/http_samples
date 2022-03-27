/*
 * In this first example, we are going to make a GET request to the server and
 * then we store its response in a file.
 *
 * Note: I don't deal with memory errors since it's an example,
 * some others errors can also be ommited for simplicity.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h> /* for close() */

#include <sys/socket.h> /* for socket() and sockaddr */
#include <arpa/inet.h>  /* for inet_ntop() */
#include <netdb.h>      /* for  addrinfo and getaddrinfo() */

#define SERVERHOST "www.example.org"
#define PORT 80

/* Given a HOSTNAME and a PORT returns the matching SOCKADDR
 * for that host and store the SOCKADDR length into ADDR_LEN. */
static struct sockaddr *get_sockaddr (const char *hostname, uint16_t port,
                                      socklen_t *addr_len);

static void write_to_server (int fd);
static char *read_from_server (int fd);

int
main ()
{
  int sock;
  struct sockaddr *server_addr;
  socklen_t addr_size;

  server_addr = get_sockaddr (SERVERHOST, PORT, &addr_size);

  if (!server_addr)
    {
      fprintf (stderr, "Could not locate %s\n", SERVERHOST);
      return EXIT_FAILURE;
    }

  /* Create the socket, either IPv4 or IPv6. */
  if (server_addr->sa_family == PF_INET)
      sock = socket (PF_INET, SOCK_STREAM, 0);
  else
    sock = socket (PF_INET6, SOCK_STREAM, 0);
  
  if (sock < 0)
    {
      perror ("socket");
      return EXIT_FAILURE;
    }

  /* Connect to the server. */
  if (connect (sock, server_addr, addr_size) < 0)
    {
      perror ("connect (client)");
      close (sock);
      return EXIT_FAILURE;
    }
  
  /* Send data to the server. */
  write_to_server (sock);

  /* Getting the response. */
  char *res = read_from_server (sock);

  if (res)
    {
      /* Create a file to store the response. */
      FILE *out = fopen ("response.txt", "w");
      
      if (!out)
	{
	  perror ("failed to create file: response.txt");
	  close (sock);
	  free (res);
	  return EXIT_FAILURE;
	}

      /* Writing the response to the file. */
      if (fwrite (res, sizeof (char), strlen (res), out) < 0)
        perror ("fwrite to: response.txt failed");

      free (res);
      fclose (out);
    }
  else
    fprintf (stderr, "Failed to retrieve a response\n");

  close (sock);
  return 0;
}

static struct sockaddr *
get_sockaddr (const char *hostname, uint16_t port, socklen_t *addr_len)
{
  struct sockaddr *result;
  struct addrinfo hints = {};
  struct addrinfo *hostinfo;
  char port_str[4];

  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  
  sprintf (port_str, "%d", port);
  if (getaddrinfo (hostname, port_str, &hints, &hostinfo) != 0)
    {
      perror ("getaddrinfo");
      return NULL;
    }

  if (addr_len)
    *addr_len = hostinfo->ai_addrlen;
  
  result = malloc (hostinfo->ai_addrlen);
  memcpy (result, hostinfo->ai_addr, hostinfo->ai_addrlen);

  /* debug */
  {
    char addr_str[100];
    void *ptr;
    
    if (hostinfo->ai_family == AF_INET)
      ptr = &((struct sockaddr_in *)result)->sin_addr;
    else
      ptr = &((struct sockaddr_in6 *)result)->sin6_addr;
    
    inet_ntop (result->sa_family, ptr, addr_str, 100);
    puts (addr_str);
  }

  freeaddrinfo (hostinfo);
  return result;
}

static void
write_to_server (int fd)
{
  int nbytes;
  const char *req =
    "GET / HTTP/1.0\n"
    "From: example@ex.com\n"
    "User-Agent: HTTPTool/1.0\n"
    "\n";

  nbytes = write (fd, req, strlen (req));
  if (nbytes < 0)
    perror ("write");
}

static char *
read_from_server (int fd)
{
  int nbytes;
  
  char *buf;
  char *buf_ptr;
  size_t bufsize = 100;
  size_t remain_size = bufsize;

  buf_ptr = buf = malloc (bufsize);

  while ((nbytes = read (fd, buf_ptr, remain_size)) >= 0)
    {
      if (nbytes < remain_size)
	{
	  size_t final_size = bufsize + nbytes;
	  buf = realloc (buf, final_size);
	  buf[final_size - 1] = '\0';
	  return buf;
	}
      else
	{
	  size_t oldsize = bufsize;
	  
	  bufsize *= 2;
	  buf = realloc (buf, bufsize);
	  buf_ptr = &buf[oldsize];

	  remain_size = oldsize;
	}
    }
  
  perror ("read");
  free (buf);
  return NULL;
}


