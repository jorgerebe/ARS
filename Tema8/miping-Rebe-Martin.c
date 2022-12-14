// Practica tema 8, Rebe Martin Jorge

#include "ip-icmp-ping.h"

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

#define DEBUG 0

#define BUF_SIZE 128
#define PAYLOAD "Este es el payload"

void mensaje(char* cadena);

int main(int argc, char* argv[]){

   int sockfd;
   bool verbose = false;
   struct sockaddr_in addr_server;

   int opt;
   char* ip;
   char* request;

   /*
    * 	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    * 	comprobando el numero de argumentos utilizados
    */

   if(argc < 2 || argc > 3){
      fprintf(stderr, "usage: %s direccion-ip [-v]\n", argv[0]);
      exit(EXIT_FAILURE);
   }


   /**
    *	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    *	comprobando que las opciones utilizadas son correctas (opcion -r -w y -v)
    *
    *	Si se utiliza alguna otra, se muestra el error y se sale.
    */


   while((opt = getopt(argc, argv, "v")) != -1){
      switch(opt){
         case 'v':
            verbose = true;
	    break;
         default:
      	   fprintf(stderr, "usage: %s direccion-ip [-v]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
   }

   ip = argv[optind];

#if DEBUG
   printf("IP: %s\n", ip);
   printf("MODO: %d\n", opcode);
   printf("ARCHIVO: %s\n", fileName);
   printf("VERBOSE: %s\n", verbose?"true":"false");
#endif


   /*
    *   Generacion del descriptor del socket UDP
    */

   struct sockaddr_in myaddr;


   sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
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

   /*
    *	Se busca la direccion realizar la comunicacion.
    */

   /*
    * 	En primer lugar, se obtiene la direccion ip del argumento al que apunta argv[optind] [getopt(3)]
    * 	optind es el indice en argv del primer elemento de ese vector que no es una opcion
    */

   ip = argv[optind];
   struct in_addr ipServer;
  
   /*
    *	Convertimos la cadena que contiene la IP en un numero de 32 bits en network byte order, y
    *	lo guarda en una estructura in_addr
    */

   if(inet_aton(ip, &ipServer) == 0){
      fprintf(stderr, "Invalid address\n");
      exit(EXIT_FAILURE);
   }


   /*
    *	Se asigna los datos de la direccion del servidor a los campos
    *	de la estructura sockaddr_in
    */


   addr_server.sin_family = AF_INET;
   addr_server.sin_port = 0;
   addr_server.sin_addr.s_addr = ipServer.s_addr;


#if debug
   printf("Pre-envio\n");
   fflush(stdout);
#endif

   /*
    * Se crea la solicitud para el servidor
    */

   ECHORequest echoRequest;
   ICMPHeader icmpHeader;

   icmpHeader.Type = 8;
   icmpHeader.Code = 0;
   icmpHeader.Checksum = 0;
   echoRequest.icmpHeader = icmpHeader;
   echoRequest.ID = 1000;//getpid();
   echoRequest.SeqNumber = 0;
   strncpy(echoRequest.payload, PAYLOAD, strlen(PAYLOAD));

   int numShorts = sizeof(echoRequest)/2;
   unsigned short int *puntero;
   unsigned int acumulador = 0;

   puntero = (unsigned short int *)&echoRequest;

   int i;

   for(i = 0; i < numShorts; i++){
      printf("Dir: %p\n", puntero);
      acumulador = acumulador + (unsigned int) *puntero;
      printf("%d - %d\n", i, *puntero);
      puntero++;
   }

   acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
   acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);

   printf("Acumulador: %hu\n", acumulador);

   return 0;


   /*
    *	Se envia un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int envio = sendto(sockfd, request, BUF_SIZE, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(envio == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }


   if(verbose){
   }


#if DEBUG
   printf("Datagram sent. Size: %s\n", request+2);
   fflush(stdout);
#endif



   /*
    * Cierre del socket
    */

  int cierre = close(sockfd);

  if(cierre < 0){
     perror("close()");
     exit(EXIT_FAILURE);
  }

  /*
   *	Finalizacion del cliente con exito
   */

   exit(EXIT_SUCCESS);
}

void mensaje(char* cadena){
   printf("->%s.\n", cadena);
}
