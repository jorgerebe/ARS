// Practica tema 5, Reb√© Mar√n Jorge

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <endian.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <netdb.h>

#define DEBUG 0

#define BUF_SIZE 50

#define NOMBRE_SERVICIO "daytime"
FILE *fich;

int main(int argc, char* argv[]){

   char hostname[BUF_SIZE];

   int error = gethostname(hostname, BUF_SIZE);

   if(error < 0){
      perror("gethostname");
      exit(EXIT_FAILURE);
   }

   strcat(hostname, ": ");

   FILE *fich;

   int sizeHostname = strlen(hostname);

   int opt;
   int puerto_tmp = -1;
   uint16_t puerto;

   /*   Comprobacio√n de que la llamada al programase ha realizado de manera correct,
    *   comprobando el numero de argumentos utilizados
    */

   if(argc == 2 || argc > 3){
      fprintf(stderr, "Usage: %s [-p puerto-servidor]\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   /*  
    *  Si se ha utilizado la opcio√n -p, se recoge su valor.
    *  Si se ha utilizado una opcio√n no v√lida, se reporta el error
    */

   while((opt = getopt(argc, argv, "p:")) != -1){
      switch(opt){
         case 'p':
            puerto_tmp = atoi(optarg);
            puerto = htons(puerto_tmp);
            break;
         default:
            fprintf(stderr, "Usage: %s [-p puerto-servidor]\n", argv[0]);
            exit(EXIT_FAILURE);
      }
   }

   /* Si no se ha especificado puerto, se busca el puerto bien conocido del
    * servicio ofrecido (daytime)
    */

   struct servent* info_servidor;

   if(puerto_tmp == -1){

      info_servidor = getservbyname(NOMBRE_SERVICIO, NULL);

      if(info_servidor == NULL){
         printf("Servidor no encontrado\n");
         exit(EXIT_FAILURE);
      }

      puerto = info_servidor->s_port;
   }
   
   /*Creacio√n del socket*/

   int sockfd;
   struct sockaddr_in myaddr;

   sockfd = socket(AF_INET, SOCK_DGRAM, 0);
   if(sockfd < 0){
      perror("socket()");
      exit(EXIT_FAILURE);
   }

   /*Enlace del socket a direccio√n local*/

   myaddr.sin_family = AF_INET;
   myaddr.sin_port = puerto;
   myaddr.sin_addr.s_addr = INADDR_ANY;

   error = bind(sockfd, (struct sockaddr *) &myaddr, sizeof(myaddr));

   if(error < 0){
      perror("bind()");
      exit(EXIT_FAILURE);
   }

#if DEBUG
   printf("Socket binded\n");
#endif


   struct sockaddr_in addr_sender;
   addr_sender.sin_family = AF_INET;


#if DEBUG
   printf("Puerto:%d\n", puerto);
   fflush(stdout);
#endif

   char in_datagram[BUF_SIZE];
   int nread;
   int envio;
   socklen_t sizeAddress = sizeof(struct sockaddr_storage);
   char* buf;


/*Bucle de recibimiento de solicitudes*/
   for(;;){
      
      buf = (char *)malloc(BUF_SIZE*sizeof(char));

      strcat(buf, hostname);

#if debug
      printf("Esperando datagrama\n");
      fflush(stdout);
#endif

      nread = recvfrom(sockfd, in_datagram, BUF_SIZE, 0, (struct sockaddr*)&addr_sender, &sizeAddress);

      /*
       *   Si hay una peticio√n fallida, se ignora
       */

      if(nread == -1){
         continue;
      }

#if DEBUG
      printf("FAMILY: %d\n",addr_sender.sin_family);
      printf("PORT: %d\n", addr_sender.sin_port);
      printf("ADDR: %d\n\n", addr_sender.sin_addr.s_addr);

      printf("El cliente dijo: %s\n", in_datagram);

      fflush(stdout);
#endif   
      
      /*
       *   Se obtiene la fecha y la hora utilizando al funcion system,
       *   ejecutando el comando shell 'date' y redirigiendo su salida
       *   a un fichero que se encontrara en la direccion /tmp/tt.txt,
       *   sobreescribiendolo si ya existiera.
       *
       *   Despues, se escribe el resultado en el buffer buf
       */


      system("date > /tmp/tt.txt");
      fich = fopen("/tmp/tt.txt", "r");
      if(fgets(buf+sizeHostname*sizeof(char), BUF_SIZE, fich) == NULL){
         fprintf(stderr, "Error en system(), en fopen(), o en fgets()\n");
         exit(EXIT_FAILURE);
      }

      fclose(fich);

     /*
      *   Se concatenan la cadena que contiene el nombre del host con
      *   la cadena que contiene la fecha y la hora
      */ 

#if DEBUG
      printf("%s\n",hostname);
#endif

      envio = sendto(sockfd, buf, BUF_SIZE, 0, (struct sockaddr*)&addr_sender, sizeAddress);
      if(envio == -1){
         perror("sendto()");
         exit(EXIT_FAILURE);
      }

      free(buf);
   }

}
