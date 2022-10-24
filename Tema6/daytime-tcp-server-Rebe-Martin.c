// Practica tema 5, Rebe Martin Jorge


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

#include <endian.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include <netdb.h>

#define DEBUG 0

#define BUF_SIZE 50
#define MAX_CONEXIONES 5
#define NOMBRE_SERVICIO "daytime"

void salir(char*argv[]);


int main(int argc, char* argv[]){

   char hostname[BUF_SIZE];
   int child;
   int error = gethostname(hostname, BUF_SIZE);

   if(error < 0){
      perror("gethostname");
      exit(EXIT_FAILURE);
   }

   strcat(hostname, ": ");


   int sizeHostname = strlen(hostname);

   int opt;
   int puerto_tmp = -1;
   uint16_t puerto;

   /*   Comprobacio�n de que la llamada al program ase ha realizado de manera correct,
    *   comprobando el numero de argumentos utilizados
    */

   if(argc == 2 || argc > 3){
      salir(argv);
   }

   /*  
    *  Si se ha utilizado la opcio�n -p, se recoge su valor.
    *  Si se ha utilizado una opcio�n no a�lida, se reporta el erro
    */

   while((opt = getopt(argc, argv, "p:")) != -1){
      switch(opt){
         case 'p':
            puerto_tmp = atoi(optarg);
            if(puerto_tmp < 0){
               salir(argv);
            }
            puerto = htons(puerto_tmp);
            break;
         default:
           salir(argv);
      }
   }

   /* Si no se ha especificado puerto, se busca el puerto bien conocido del
    * servicio ofrecido (daytime)
    */

   struct servent* info_servidor;

   if(puerto_tmp == -1){

      info_servidor = getservbyname(NOMBRE_SERVICIO, NULL);

      if(info_servidor == NULL){
         printf("Servicio no encontrado\n");
         exit(EXIT_FAILURE);
      }

      puerto = info_servidor->s_port;
   }
   
   /*Creacio�n del socket*/

   int sockfd;
   struct sockaddr_in myaddr;

   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   if(sockfd < 0){
      perror("socket()");
      exit(EXIT_FAILURE);
   }

   /*Enlace del socket a direccio�n local*/

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
   printf("Puerto:%d\n", be16toh(puerto));
   fflush(stdout);
#endif

   socklen_t sizeAddress = sizeof(struct sockaddr_storage);
   char* buf;
   int socket_conexion;

   error = listen(sockfd, MAX_CONEXIONES);

   if(error < 0){
      perror("listen()");
      exit(EXIT_FAILURE);
   }


/*Bucle de recibimiento de solicitudes*/
   while(true){

      /*
       * Se reserva espacio para un buffer en el que
       * se mandara la respuesta al cliente
       *
       */
      
      buf = (char *)malloc(BUF_SIZE*sizeof(char));
      buf[0] = '\0';

      strcat(buf, hostname);

#if debug
      printf("Esperando datagrama\n");
      fflush(stdout);
#endif

      socket_conexion = accept(sockfd, (struct sockaddr*)&addr_sender, &sizeAddress);

      if(socket_conexion < 0){
         perror("accept()");
         continue;
      }

      child = fork();

      if(child != 0){
         continue;
      }
      else{

         FILE *fich;
         int envio;

         system("date > /tmp/tt.txt");
      	 fich = fopen("/tmp/tt.txt", "r");
      	 if(fgets(buf+sizeHostname*sizeof(char), BUF_SIZE, fich) == NULL){
             fprintf(stderr, "Error en system(), en fopen(), o en fget\n");
             exit(EXIT_FAILURE);
          }

       fclose(fich);

       envio = send(socket_conexion, buf, BUF_SIZE, 0);
       if(envio == -1){
            perror("sendto()");
            exit(EXIT_FAILURE);
       }

       exit(0);
      }


      /*
       *   Si hay una peticio�n fallida, se igora
       */

      
      /*
       *   Se obtiene la fecha y la hora utilizando al funcion system,
       *   ejecutando el comando shell 'date' y redirigiendo su salida
       *   a un fichero que se encontrara en la direccion /tmp/tt.txt,
       *   sobreescribiendolo si ya existiera.
       *
       *   Despues, se escribe el resultado en el buffer buf despues
       *   del nombre del host
       */
     

#if DEBUG
      printf("Mensaje enviado: %s\n",buf);
#endif

      /*
       * Se envia la respuesta al cliente. Si hay error en el envio, se reporta
       * y se termina el programa
       */

      

      /*
       * Se libera el espacio ocupado por el buffer
       * de la respuesta al cliente
       */

      free(buf);
   }

}

void salir(char* argv[]){
   fprintf(stderr, "Usage: %s [-p puerto-servidor]\n", argv[0]);
   exit(EXIT_FAILURE);
}
