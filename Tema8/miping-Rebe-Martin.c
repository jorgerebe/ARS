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

#define PAYLOAD_SIZE 64
#define TYPE_PING 8
#define CODE_PING 0
#define PAYLOAD "Este es el payload."

unsigned short int getChecksum(ECHORequest echoRequest);

int main(int argc, char* argv[]){

   int sockfd;
   bool verbose = false;
   struct sockaddr_in addr_server;

   int opt;
   char* ip;

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
    *	comprobando que las opciones utilizadas son correctas (solo opcion -v)
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
   printf("VERBOSE: %s\n", verbose?"true":"false");
#endif


   /*
    *   Generacion del descriptor del socket RAW
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
    *	Convertimos la cadena que contiene la IP en un numero de 32 bits en
    *	network byte order, y lo guarda en una estructura in_addr
    */

   if(inet_aton(ip, &ipServer) == 0){
      fprintf(stderr, "Invalid address\n");
      exit(EXIT_FAILURE);
   }


   /*
    *	Se asigna los datos de la direccion del servidor a los campos
    *	de la estructura sockaddr_in.
    *
    *	El puerto se pone a 0 porque es indiferente. El destino de un
    *	datagrama ICMP no es una aplicacion de las capas superiores,
    *	sino la capa IP de la maquina destino.
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

   icmpHeader.Type = TYPE_PING;
   icmpHeader.Code = CODE_PING;
   icmpHeader.Checksum = 0;
   echoRequest.icmpHeader = icmpHeader;
   echoRequest.ID = getpid();
   echoRequest.SeqNumber = 0;
   strncpy(echoRequest.payload, PAYLOAD, PAYLOAD_SIZE);

   /*
    * Calculo del checksum del datagrama ICMP y de la comprobacion de si ha sido
    * calculado correctamente.
    */

   unsigned short int checksum = getChecksum(echoRequest);

   echoRequest.icmpHeader.Checksum = checksum;

   if(getChecksum(echoRequest) != 0){
      fprintf(stderr, "Error al calcular el checksum");
      exit(EXIT_FAILURE);
   }

   /*
    * Se imprimen los valores de la cabecera ICMP y del cuerpo del datagrama
    * ICMP si el cliente ha elegido el modo 'verbose'
    */

   if(verbose){
      printf("-> Generando cabecera ICMP.\n");
      printf("-> Type: %d\n", echoRequest.icmpHeader.Type);
      printf("-> Code: %d\n", echoRequest.icmpHeader.Code);
      printf("-> Identifier (pid): %d.\n", echoRequest.ID);
      printf("-> Seq. number: %d\n", echoRequest.SeqNumber);
      printf("-> Cadena a enviar: %s\n", echoRequest.payload);
      printf("-> Checksum: 0x%x.\n", echoRequest.icmpHeader.Checksum);
      printf("-> Tamano total del paquete ICMP: %ld.\n", sizeof(echoRequest));
   }

   /*
    * Envio del datagrama ICMP a la maquina destino
    */

   int envio = sendto(sockfd, &echoRequest, sizeof(echoRequest), 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(envio == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }

   printf("Paquete ICMP enviado a %s\n", inet_ntoa(addr_server.sin_addr));


#if DEBUG
   printf("Datagram sent. Size: %ld\n", sizeof(echoRequest));
   fflush(stdout);
#endif

   ECHOResponse echoResponse;
   socklen_t addrlen = sizeof(struct sockaddr);

   /*
    * Recepcion del datagrama ICMP de respuesta
    */

   int respuesta = recvfrom(sockfd, &echoResponse, sizeof(echoResponse), 0, (struct sockaddr*)&addr_server, &addrlen);

   if(respuesta == -1){
      perror("recvfrom()");
      exit(EXIT_FAILURE);
   }

   printf("Respuesta recibida desde %s\n", inet_ntoa(addr_server.sin_addr));


   /*
    * Si el usuario ha solicitado la opcion 'verbose', se imprime informacion
    * adicional. Entre esta informacion, esta el contador TTL presente en la
    * cabecera ICMP
    */

   if(verbose){
      printf("-> Tamano de la respuesta: %ld\n", sizeof(echoResponse));
      printf("-> Cadena recibida: %s\n", echoResponse.payload);
      printf("-> Identifier (pid): %d\n", echoResponse.ID);
      printf("-> TTL: %d.\n", (unsigned short int)echoResponse.ipHeader.TTL);
   }


   /*
    * Impresion por pantalla del tipo de la respuesta y el codigo,
    * y descripcion de la respuesta de acuerdo a ese codigo: se exponen
    * algunos errores que pueden ser los mas comunes, pero puede haber
    * otros no descritos aqui.
    */

   short int type = echoResponse.icmpHeader.Type;
   short int code = echoResponse.icmpHeader.Code;

   printf("Descripcion de la respuesta: ");

   if(type == 0 && code == 0){
      printf("respuesta correcta ");
   }
   else{
      switch(type)
      {
         case 3:
            printf("Destination unreachable: ");
            switch(code)
            {
               case 0:
                  printf("Destination network unreachable ");
                  break;
               case 1:
		            printf("Destination host unreachable ");
		            break;
               case 2:
                  printf("Destination protocol unreachable ");
                  break;
               case 3:
                  printf("Destination port unreachable ");
                  break;
               case 9:
                  printf("Network administratively prohibited ");
                  break;
               case 10:
                  printf("Host administratively prohibited ");
                  break;
               case 11:
                  printf("Network unreachable for ToS ");
                  break;
               case 12:
                  printf("Host unreachable for ToS ");
                  break;
               case 13:
                  printf("Communication administratively prohibited ");
                  break;
            }
            break;
         case 5:
            printf("Redirect Message: ");
            switch(code)
            {
               case 0:
                  printf("Redirect Datagram for the Network");
                  break;
               case 1:
                  printf("Redirect Datagram for the Host");
                  break;
               case 2:
                  printf("Redirect Datafram for the ToS & network");
                  break;
               case 3:
                  printf("Redirect Datagram for the ToS & host ");
                  break;
            }
            break;
         case 8:
            printf("Echo Request: Echo request (used to ping) ");
            break;
         case 11:
            switch(code)
            {
               case 0:
                  printf("TTL expired in transit ");
                  break;
               case 1:
                  printf("Fragment reassembly time exceeded ");
                  break;
            }
            break;
      }
   }

   printf("(type %d, code %d).\n", type, code);


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


unsigned short int getChecksum(ECHORequest echoRequest){

   /*
    * Funcion para calcular el checksum de una peticion
    */

   /*
    * Calculo del numero de palabras de 16 bits
    */

   int numShorts = sizeof(echoRequest)/2;

#if DEBUG
   printf("Num shorts: %d\n", numShorts);
   fflush(stdout);
#endif

   /*
    * Definicion de un puntero que apunta al inicio de la estructura
    * ECHORequest y un acumulador inicializado a 0
    */

   unsigned short int *puntero;
   unsigned int acumulador = 0;
   int i = 0;

   puntero = (unsigned short int *)&echoRequest;

   /*
    * Sumamos el valor de cada palabra de 2 bytes al acumulador.
    * Incrementamos el puntero (cada incremento, es un incremento de 2 bytes
    * porque el puntero apunta a una variable de 2 bytes)
    */

   for(i = 0; i < numShorts; i++){
      acumulador = acumulador + (unsigned int) *puntero;
      puntero++;
   }

   /*
    * Sumamos la parte alta del acumulador a la parte baja
    * (para que la suma sea correcta con 16 bit en complemento a uno)
    */

   acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);
   acumulador = (acumulador >> 16) + (acumulador & 0x0000ffff);

   /*
    * Por ultimo, hacemos un NOT logico al acumulador para obtener
    * su complemento a uno.
    */

   acumulador = ~acumulador;

#if DEBUG
   printf("Checksum: %x\n", (short int)acumulador);
   fflush(stdout);
#endif

   /*
    * Devolvemos los 16 bits de menos peso del acumulador, que
    * sera el checksum
    */

   return (unsigned short int) acumulador;

}
