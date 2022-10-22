// Practica tema 5, Rebé Mar�n Jorge

#include <errno.h>
#include<stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <endian.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <netdb.h>

#define DEBUG 0

#define BUF_SIZE 50 

#define NOMBRE_SERVICIO "daytime"

int main(int argc, char* argv[]){

   char buf[BUF_SIZE] = "Solicitud de servicio";
   int opt;
   int puerto_tmp = -1;
   uint16_t puerto;

   char* ip;

   /*
    * 	Comprobacio�n de que la llamada al programa se ha realizado de manera correcta,
    * 	comprobando el nu�mero de argumentos utilizados
    */

   if(argc < 2 || argc == 3 || argc > 4){
      fprintf(stderr, "Usage: %s direccion.IP.servidor [-p puerto-servidor]\n", argv[0]);
      exit(EXIT_FAILURE);
   }


   /**
    *	Comprobacio�n de que la llamada al programa se ha realizado de manera correcta,
    *	comprobando que las opciones utilizadas son correctas (s�lo opc�n -p)
    *
    *	Si se utiliza alguna otra, se muestra el error y se sale.
    */


   while((opt = getopt(argc, argv, "p:")) != -1){
      switch(opt){
         case 'p':
            puerto_tmp = atoi(optarg);
            puerto = htons(puerto_tmp);
            break;
         default:
            fprintf(stderr, "Usage: %s direccion.IP.servidor [-p puerto-servidor]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
   }
   


   /*
    *	Generació��n del descriptor del socket UDP
    */

   int sockfd;
   struct sockaddr_in myaddr;


   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if(sockfd < 0){
      perror("socket()");
      exit(EXIT_FAILURE);
   }

   /*
    *	Enlace del socket a una direcci�n y a un puerto locales.
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
    *	Se busca la direcci�n IP y el n�mero de puerto de protocolo del servidor con el que se desea
    *	realizar la comunicaci�n. S
    */

   /**
    * 	En primer lugar, se obtiene la direcci�n ip del argumento al que apunt argv[optind] [getopt(3)]
    * 	optind es el �ndice en argv del primer elemento de ese vector que no es una opci�n
    */

   ip = argv[optind];
   struct in_addr ipServer;
   struct sockaddr_in addr_server;
   struct servent* info_servidor;

   /**
    *	Convertimos la cadena que contiene la IP en un n�mero de 32 bits en network byte order, y
    *	lo guarda en una estructura in_addr
   **/

   if(inet_aton(ip, &ipServer) == 0){
      fprintf(stderr, "Invalid address\n");
      exit(EXIT_FAILURE);
   }


   /*
    *	Si no se ha especificado puerto, se busca el correspondiente
    *	al servicio que se va a utilizar, en este caso, daytime; utlizando
    *	para ello getservbyname()
    */

   if(puerto_tmp == -1){

      info_servidor = getservbyname(NOMBRE_SERVICIO, NULL);

      if(info_servidor == NULL){
#if DEBUG
         printf("Servidor no encontrado\n");
#endif
         exit(EXIT_FAILURE);
      }

      puerto = info_servidor->s_port;
   }

#if DEBUG
   printf("Puerto:%hu\n", be16toh(puerto));
   printf("Server found\n");
   fflush(stdout);
#endif
   
   /*
    *	Se asigna los datos de la direcci�n del servidor a los campos
    *	de la estructura sockaddr_in
    */


   addr_server.sin_family = AF_INET;
   addr_server.sin_port = puerto;
   addr_server.sin_addr.s_addr = ipServer.s_addr;


#if debug
   printf("Pre-envio\n");
   fflush(stdout);
#endif


   /**
    *	Se env�a un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int envio = sendto(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

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
   *	Finalizaci�n del cliente
   */

   exit(EXIT_SUCCESS);

}
