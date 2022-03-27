/*
 * Esse primeiro exemplo realiza uma requisição do tipo GET para um servidor
 * e então armazena sua resposta em um arquivo.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <unistd.h>

#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SERVERHOST "www.example.org"
#define PORT 80

static void init_sockaddr (struct sockaddr_in *name,
			   const char *hostname,
                           uint16_t port);

static void write_to_server (int fd);
static char *read_from_server (int fd);

int
main ()
{
  int sock;
  struct sockaddr_in servername;
  
  /* Create the socket. */
  sock = socket (PF_INET, SOCK_STREAM, 0);
  if (sock < 0)
    {
      perror ("socket");
      return EXIT_FAILURE;
    }

  /* Connect to the server. */
  init_sockaddr (&servername, SERVERHOST, PORT);
  if (connect (sock, (struct sockaddr *)&servername, sizeof (servername)) < 0)
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
      FILE *out = fopen ("response.txt", "w");
      int out_fd;
      
      if (!out)
	{
	  perror ("failed to create file: response.txt");
	  close (sock);
	  free (res);
	  return EXIT_FAILURE;
	}

      /* Writing the response in a file. */
      out_fd = fileno (out);
      if ((write (out_fd, res, strlen (res))) < 0)
        perror ("write to: response.txt failed");

      free (res);
      fclose (out);
    }
  else
    fprintf (stderr, "Failed to retrieve a response\n");

  close (sock);
  return 0;
}

static void
init_sockaddr (struct sockaddr_in *name, const char *hostname, uint16_t port)
{
  struct hostent *hostinfo;

  /* Setting the adrress family to the IPv4 format. */
  name->sin_family = AF_INET;
  /* htons() convert a host endianess to the internet endianess. */
  name->sin_port = htons (port);

  /* Getting the host information, its IPv4 adrress.*/
  hostinfo = gethostbyname (hostname);
  if (!hostinfo)
    {
      fprintf (stderr, "Unknown host %s\n", hostname);
      return;
    }

  name->sin_addr = *(struct in_addr *)hostinfo->h_addr;
}

static void
write_to_server (int fd)
{
  int nbytes;
  const char *req =
    "GET /index.html HTTP/1.0\n"
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
      if (nbytes == 0)
	return buf;
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

