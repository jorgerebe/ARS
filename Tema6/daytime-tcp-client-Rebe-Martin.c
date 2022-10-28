// Practica tema 6, Rebe Martin Jorge


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

   char buf[BUF_SIZE];
   int opt;
   int puerto_tmp = -1;
   uint16_t puerto;

   char* ip;

   /*
    * 	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    * 	comprobando el numero de argumentos utilizados
    */

   if(argc < 2 || argc == 3 || argc > 4){
      fprintf(stderr, "Usage: %s direccion.IP.servidor [-p puerto-servidor]\n", argv[0]);
      exit(EXIT_FAILURE);
   }


   /**
    *	Comprobacion de que la llamada al programa se ha realizado de manera correcta,
    *	comprobando que las opciones utilizadas son correctas (solo opcion -p)
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
    *	Generacion del descriptor del socket TCP
    */

   int sockfd;
   struct sockaddr_in myaddr;


   sockfd = socket(AF_INET, SOCK_STREAM, 0);
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

   if(puerto_tmp == -1){

      info_servidor = getservbyname(NOMBRE_SERVICIO, NULL);

      if(info_servidor == NULL){
#if DEBUG
         printf("Servicio no encontrado\n");
#endif
         exit(EXIT_FAILURE);
      }

      puerto = info_servidor->s_port;
   }

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
   printf("Pre-conexion\n");
   fflush(stdout);
#endif


   /**
    *	Se envia un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int conexion = connect(sockfd, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(conexion == -1){
      perror("connect()");
      exit(EXIT_FAILURE);
   }

#if debug
   printf("Conexion establecida.\n");
   fflush(stdout);
#endif



   /*
    *	Se recibe la respuesta del servidor y se imprime por pantalla
    */

   int respuesta =recv(sockfd, buf, BUF_SIZE, 0);

  if(respuesta == -1){
     perror("recv()");
     exit(EXIT_FAILURE);
   }
   
   printf("%s", buf);

   /*
    *	Cierre del socket UDP
    */
   shutdown(sockfd, SHUT_RDWR);
   
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
