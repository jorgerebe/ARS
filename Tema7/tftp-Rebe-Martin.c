// Practica tema 7, Rebe Martin Jorge


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <endian.h>

#include <stdbool.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <netdb.h>

#define DEBUG 1

#define BUF_SIZE 128

#define OPCODE_RRQ  1
#define OPCODE_WRQ  2
#define OPCODE_DATA 3
#define OPCODE_ACK  4

#define NOMBRE_SERVICIO "tftp"
#define MODO "octet"

int main(int argc, char* argv[]){

   char* file;

   bool verbose = false;
   uint16_t puerto;

   int opt;
   char* ip;
   short int opcode;
   char* request;

   /*
    * 	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    * 	comprobando el numero de argumentos utilizados
    */

   if(argc < 4 || argc > 5){
      fprintf(stderr, "Usage: %s ip-servidor {-r|-w} archivo [-v]\n", argv[0]);
      exit(EXIT_FAILURE);
   }


   /**
    *	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    *	comprobando que las opciones utilizadas son correctas (solo opcion -p)
    *
    *	Si se utiliza alguna otra, se muestra el error y se sale.
    */


   while((opt = getopt(argc, argv, "rwv")) != -1){
      switch(opt){
         case 'r':
            file = argv[optind];
            opcode = OPCODE_RRQ;
            break;
         case 'w':
            file = argv[optind];
            opcode = OPCODE_WRQ;
	    break;
         case 'v':
            verbose = true;
	    break;
         default:
            fprintf(stderr, "Usage: %s ip-servidor {-r|-w} archivo [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
   }

   ip = argv[optind];

#if DEBUG
   printf("IP: %s\n", ip);
   printf("MODO: %d\n", request);
   printf("ARCHIVO: %s\n", file);
   printf("VERBOSE: %s\n", verbose?"true":"false");
#endif


   /*
    *	Generacion del descriptor del socket UDP
    */

   int sockfd;
   struct sockaddr_in myaddr;


   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if(sockfd < 0){
      perror("socket()");
      exit(EXIT_FAILURE);
   }

   /*
    *	Enlace del socket a una direccion y a un puerto locales.
    *	El puerto se pone a 0 para que el sistema operativo lo decida
    */


   myaddr.sin_family = AF_INET;
   myaddr.sin_port = 0;
   myaddr.sin_addr.s_addr = INADDR_ANY;

   int error = bind(sockfd, (struct sockaddr *) &myaddr, sizeof(myaddr));

   if(error < 0){
      perror("bind()");
      exit(EXIT_FAILURE);
   }

#if DEBUG
   printf("Socket binded\n");
#endif

   /**
    *	Se busca la direccion IP y el numero de puerto de protocolo del servidor con el que se desea
    *	realizar la comunicacion. S
    */

   /**
    * 	En primer lugar, se obtiene la direccion ip del argumento al que apunta argv[optind] [getopt(3)]
    * 	optind es el indice en argv del primer elemento de ese vector que no es una opcion
    */

   ip = argv[optind];
   struct in_addr ipServer;
   struct sockaddr_in addr_server;
   struct servent* info_servidor;

   /**
    *	Convertimos la cadena que contiene la IP en un numero de 32 bits en network byte order, y
    *	lo guarda en una estructura in_addr
   **/

   if(inet_aton(ip, &ipServer) == 0){
      fprintf(stderr, "Invalid address\n");
      exit(EXIT_FAILURE);
   }


   /*
    *	Si no se ha especificado puerto, se busca el correspondiente
    *	al servicio que se va a utilizar, en este caso, daytime; utilizando
    *	para ello getservbyname()
    */

   info_servidor = getservbyname(NOMBRE_SERVICIO, NULL);

   if(info_servidor == NULL){
#if DEBUG
      printf("Servicio no encontrado\n");
#endif
      exit(EXIT_FAILURE);
   }

   puerto = info_servidor->s_port;

#if DEBUG
   printf("Puerto:%hu\n", be16toh(puerto));
   fflush(stdout);
#endif
   
   /*
    *	Se asigna los datos de la direccion del servidor a los campos
    *	de la estructura sockaddr_in
    */


   addr_server.sin_family = AF_INET;
   addr_server.sin_port = puerto;
   addr_server.sin_addr.s_addr = ipServer.s_addr;


#if debug
   printf("Pre-envio\n");
   fflush(stdout);
#endif

   /*
    * Se crea la solicitud para el servidor
    *
    */

   request = (char*)malloc(BUF_SIZE*sizeof(char));
   *request = '\0';
   request[0] = opcode;
   short int other = request[0];
   printf("%hu\n", other);
   strcpy(request+2, file);
   strcpy(request+2+strlen(file)+1, MODO);
   //printf("%s\n", request+2+strlen(file)+1);
   //printf("%s\n%d\n", request, strlen(request));
   //fflush(stdout);
   //free(request);
   //exit(EXIT_SUCCESS);



   /**
    *	Se envia un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int envio = sendto(sockfd, request, BUF_SIZE, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(envio == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }

#if debug
   printf("Datagram sent. Size: %d\n", envio);
   fflush(stdout);
#endif



   /*
    *	Se recibe la respuesta del servidor y se imprime por pantalla
    */

   socklen_t sizeRespuesta = 0;

   char buf[BUF_SIZE];

   int respuesta =recvfrom(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&addr_server, &sizeRespuesta);

  if(respuesta == -1){
     perror("recvfrom()");
     exit(EXIT_FAILURE);
   }
   
   printf("%s", buf);

   /*
    *	Cierre del socket UDP
    */

  int cierre = close(sockfd);

  if(cierre < 0){
     perror("close()");
     exit(EXIT_FAILURE);
  }

  /*
   *	Finalizacion del cliente
   */

   exit(EXIT_SUCCESS);

}
