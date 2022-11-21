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

#define DEBUG 0

#define BUF_SIZE 516
#define MAX_FILE_NAME 100

#define OPCODE_RRQ   1
#define OPCODE_WRQ   2
#define OPCODE_DATA  3
#define OPCODE_ACK   4
#define OPCODE_ERROR 5

#define NOMBRE_SERVICIO "tftp"
#define MODO "octet"

void lectura(int sockfd, struct sockaddr_in addr_server, char* fileName, char ACK[], bool verbose);
void escritura(int sockfd, struct sockaddr_in addr_server, char*fileName, bool verbose);
void sendACK(int sockfd, struct sockaddr_in addr_server, char ACK[], short int block, bool verbose);

int main(int argc, char* argv[]){

   int sockfd;
   bool verbose = false;
   struct sockaddr_in addr_server;
   char* fileName;

   /*
    * Buffer donde se enviaran los ACKS. Siempre los dos primeros bytes
    * son iguales, por lo que se establecen ahora (el numero de operacion correspondiente al ACK)
    */

   char ACK[4];
   ACK[0] = (unsigned char)OPCODE_ACK/256;
   ACK[1] = (unsigned char)OPCODE_ACK%256;

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
    *	comprobando que las opciones utilizadas son correctas (opcion -r -w y -v)
    *
    *	Si se utiliza alguna otra, se muestra el error y se sale.
    */


   while((opt = getopt(argc, argv, "rwv")) != -1){
      switch(opt){
         case 'r':
            fileName = argv[optind];
	         if(strlen(fileName) > MAX_FILE_NAME){
               fprintf(stderr, "El nombre del fichero no puede ser superior a 100 caracteres");
               exit(EXIT_FAILURE);
	         }
            opcode = OPCODE_RRQ;
            break;
         case 'w':
            fileName = argv[optind];

            /*
            * Se abre el fichero especificado para lectura
            */
   
            FILE* file = fopen(fileName, "r");

            if(file == NULL){
               fprintf(stderr, "El fichero que se quiere escribir no existe\n");
               exit(EXIT_FAILURE);
               fclose(file);
            }

            fclose(file);

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
   printf("MODO: %d\n", opcode);
   printf("ARCHIVO: %s\n", fileName);
   printf("VERBOSE: %s\n", verbose?"true":"false");
#endif


   /*
    *	Generacion del descriptor del socket UDP
    */

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

   /*
    *	Se busca la direccion IP y el numero de puerto de protocolo del servidor con el que se desea
    *	realizar la comunicacion.
    */

   /*
    * 	En primer lugar, se obtiene la direccion ip del argumento al que apunta argv[optind] [getopt(3)]
    * 	optind es el indice en argv del primer elemento de ese vector que no es una opcion
    */

   ip = argv[optind];
   struct in_addr ipServer;
  
   struct servent* info_servidor;

   /*
    *	Convertimos la cadena que contiene la IP en un numero de 32 bits en network byte order, y
    *	lo guarda en una estructura in_addr
    */

   if(inet_aton(ip, &ipServer) == 0){
      fprintf(stderr, "Invalid address\n");
      exit(EXIT_FAILURE);
   }


   /*
    *	Se busca el puerto correspondiente al servicio que se va a utilizar, en este caso, tftp; utilizando
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
    */

   request = (char*)malloc(BUF_SIZE*sizeof(char));
   *request = '\0';

   request[0] = (unsigned char)(opcode/256);
   request[1] = (unsigned char)(opcode%256);

   strcpy(request+2, fileName);
   strcpy(request+2+strlen(fileName)+1, MODO);



   /*
    *	Se envia un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int envio = sendto(sockfd, request, BUF_SIZE, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(envio == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }


   if(verbose){
      if(opcode == OPCODE_RRQ){
         printf("Enviada solicitud de lectura de %s a servidor %s en %s.\n", fileName, NOMBRE_SERVICIO, ip);
      }
      else if(opcode == OPCODE_WRQ){
         printf("Enviada solicitud de escritura de %s a servidor %s en %s.\n", fileName, NOMBRE_SERVICIO, ip);
      }
   }


#if DEBUG
   printf("Datagram sent. Size: %s\n", request+2);
   fflush(stdout);
#endif

   switch(opcode)
   {
      case OPCODE_RRQ:
         lectura(sockfd, addr_server, fileName, ACK, verbose);
         break;
      case OPCODE_WRQ:
         escritura(sockfd, addr_server, fileName, verbose);
         break;
      default:
         fprintf(stderr, "Usage: %s ip-servidor {-r|-w} archivo [-v]\n", argv[0]);
         exit(EXIT_FAILURE);
   }


   /*
    *	Cierre del socket UDP
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

void sendACK(int sockfd, struct sockaddr_in addr_server, char ACK[], short int block, bool verbose){

   /*
    * Se pone el numero de bloque correspondiente en los bytes 2 y 3 del ACK
    */

   ACK[2] = (unsigned char)(block/256);
   ACK[3] = (unsigned char)(block%256);
#if DEBUG
   printf ("ACK[0] es %d\n", ACK[0]);
   printf ("ACK[1] es %d\n", ACK[1]);
   printf ("ACK[2] es %d\n", ACK[2]);
   printf ("ACK[3] es %d\n", ACK[3]);
#endif

   /*
    * Se envia el ACK al servidor
    *
    */

   if(sendto(sockfd, ACK, 4, 0, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }

   /*
    * Se imprime la informacion sobre el ACK enviado si se ha solicitado
    */

   if(verbose){
      printf("Enviamos el ACK del bloque %d.\n", block);
   }
}


void lectura(int sockfd, struct sockaddr_in addr_server, char* fileName, char ACK[], bool verbose){

   /*
    * Se abre el fichero especificado para escritura
    */

   FILE* file = fopen(fileName, "w");

   if(file == NULL){
      perror("fopen()");
      exit(EXIT_FAILURE);
   }


   /*
    * Se inicializa la variable del size de la estructura de datos del servidor
    */

   socklen_t addrlen = sizeof(struct sockaddr);

   char* bufRespuesta;

   int respuesta;
   short int opcode_respuesta;
   short int prevBlock = -1;
   short int block = -1;

 do{
      /*
       * Se asigna espacio al bufer en el que recibiremos la informacion
       */  

      bufRespuesta = (char*)malloc(BUF_SIZE*sizeof(char));
      *bufRespuesta = '\0';

      /*
       * Se reciben los datos del servidor
       */

      respuesta = recvfrom(sockfd, bufRespuesta, BUF_SIZE, 0, (struct sockaddr*)&addr_server, &addrlen);

      if(respuesta == -1){
         perror("recvfrom()");
         exit(EXIT_FAILURE);
      }

      if(verbose){
         printf("Recibido bloque del servidor %s.\n", NOMBRE_SERVICIO);
      }

      /*
       * Se recupera el codigo de respuesta del datagrama recibido
       */

      opcode_respuesta = (unsigned char)bufRespuesta[0]*256 + (unsigned char)bufRespuesta[1];

#if DEBUG
      printf("Size respuesta: %d\n", respuesta-4);
      printf("OPCODE: %d\n", opcode_respuesta);
      fflush(stdout);
#endif


      /*
       * Se comprueba el tipo de datagrama recibido y se actua segun ello
       */

      switch(opcode_respuesta)
      {

         /*
          * Si se recibe un datagrama data, se recupera numero de bloque recibido
          */

         case OPCODE_DATA:
            block = (unsigned char)bufRespuesta[2]*256 + (unsigned char)bufRespuesta[3];

            /*
             * Comprobacion de que los paquetes se estan recibiendo en el orden correcto
             */

            if(prevBlock != -1){
               if(prevBlock + 1 != block){
                  sendACK(sockfd, addr_server, ACK, block, verbose);
                  continue;
               }

            }

            prevBlock = block;
#if DEBUG
   	      printf ("RESPUESTA[2]: %d\tRESPUESTA[3]: %d\n", (unsigned char)bufRespuesta[2], (unsigned char)bufRespuesta[3]);
            fflush(stdout);
#endif
            break;
            /*
             * Si se recibe un datagrama error, se imprime el error recibido y se sale del programa
             */
         case OPCODE_ERROR:
            fclose(file);
            short int errcode = (unsigned char)bufRespuesta[2]*256 + (unsigned char)bufRespuesta[3];
            fprintf(stderr, "errcode: %d\t%s\n", errcode, bufRespuesta+4);
            exit(EXIT_FAILURE);

            /*
             * Si no es un datagrama de data ni error, se sale del programa
             */

         default:
            fprintf(stderr, "Recibido un datagrama inesperado.\n");
            exit(EXIT_FAILURE);
      }

      /*
       * Si se ha solicitado, se imprimen los datos del bloque recibido
       */

      if(verbose){
         printf("Es el bloque con codigo %d.\n", block);
      }

      /*
       * Si el datagrama recibido es data, se escriben los datos recibidos en el fichero
       * con el nombre especificado, utilizando la funcion fwrite
       */

      fwrite(bufRespuesta+4, 1, respuesta-4, file);

      /*
       * Tras esto, se envia el ACK correspondiente al servidor
       */

      sendACK(sockfd, addr_server, ACK, block, verbose);

      free(bufRespuesta);

      /*
       * Se comprueba la condicion de salida del bucle: si se han recibido menos de 512 bytes de datos,
       * entonces la lectura del fichero ha concluido y se sale del bucle de lectura.
       */
            
   } while((respuesta-4) == 512);

   /*
    * Si se ha solicitado, se imprime los datos del ultimo bloque recibido
    */

   if(verbose){
      printf("El bloque %d era el ultimo: cerramos el fichero.\n", block);
   }

   /*
    * Se cierra el fichero
    */

   fclose(file); 
}

void escritura(int sockfd, struct sockaddr_in addr_server, char* fileName, bool verbose){

   /*
    * Se abre el fichero para lectura con el nombre especificado.
    */

   FILE* file = fopen(fileName, "r");

   if(file == NULL){
      perror("fopen()");
      exit(EXIT_FAILURE);
   }

   /*
    * Se inicializa la variable del size de la estructura de datos del servidor
    */

   socklen_t addr_len = sizeof(struct sockaddr);

   char bufRespuesta[BUF_SIZE];

   char* bufEnvio;

   int bytesSent = -1;

   int respuesta;
   short int opcode_respuesta;
   short block = 0;
   short blockACK = -1;


   /*
    * Bucle de escritura de fichero al servidor
    */

   while(true){

      /*
       * Se asigna espacio al buffer de envio de datos
       */

      bufEnvio = (char*)malloc(BUF_SIZE*sizeof(char));

      /*
       * Se espera la respuesta del ACK del servidor
       */

      respuesta = recvfrom(sockfd, bufRespuesta, BUF_SIZE, 0, (struct sockaddr*)&addr_server, &addr_len);

      if(respuesta == -1){
         perror("recvfrom()");
         exit(EXIT_FAILURE);
      }
      
      
      /*
       * Se recoge el codigo de operacion de la respuesta del servidor
       */

      opcode_respuesta = (unsigned char)bufRespuesta[0]*256 + (unsigned char)bufRespuesta[1];

      switch(opcode_respuesta)
      {
         /*
          * Si se ha recibido un ACK, se comprueba el numero de bloque. Si es el mismo al enviado
          * en el anterior envio, se comprueba el numero de bytes enviados en dicho envio.
          * Si es mayor que -1 (es decir, se ha enviado ya al menos un bloque) y menor de
          * 512 (es el ultimo envio), entonces se cierra el fichero y retornamos.
          *
          * Sino, si se ha recibido el ACK con el mismo bloque que el envio anterior, incrementamos el
          * numero de bloque. Sino, si no se ha recibido el ACK con el mismo bloque que el
          * envio anterior, se envia de nuevo el bloque anterior. 
          */

         case OPCODE_ACK:

            blockACK = (unsigned char)bufRespuesta[2]*256 + (unsigned char)bufRespuesta[3];

            if(verbose){
               printf("Recibido el ACK del bloque %d del servidor %s.\n", blockACK, NOMBRE_SERVICIO);
            }

            if(blockACK == block){
               if((bytesSent > -1) && (bytesSent < 512)){
                  if(verbose){
                     printf("El bloque %d era el ultimo: cerramos el fichero.\n", block);
                  }
                  free(bufEnvio);
                  fclose(file);
                  return;
               }
               block++;
            }
            break;
         case OPCODE_ERROR:
            fprintf(stderr, "%s\n", bufRespuesta+4);
            exit(EXIT_FAILURE);
      }

      /*
       * Se preparan los 4 primeros bytes del envio del datagrama data,
       * los 2 primeros para el codigo de operacion y los 2 segundos para el numero de bloque
       */

      bufEnvio[0] = (unsigned char)(OPCODE_DATA/256);
      bufEnvio[1] = (unsigned char)(OPCODE_DATA%256);
      bufEnvio[2] = (unsigned char)(block/256);
      bufEnvio[3] = (unsigned char)(block%256);

      /*
       * Se recogen el numero de bytes que se van a enviar en bytesSent, que es el valor que
       * retorna la funcion fread (lee el fichero y pone la cantidad de bytes especificada en el buffer de envio)
       */

      bytesSent = fread(bufEnvio+4, 1, 512, file);
      if(sendto(sockfd, bufEnvio, bytesSent+4, 0, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1){
         perror("sendto()");
         exit(EXIT_FAILURE);
      }

      /*
       * Si se ha solicitado, imprimimos la informacion del bloque de datos enviado.
       */

      if(verbose){
         printf("Enviamos el bloque con codigo %d, de %d bytes.\n", block, bytesSent);
      }

      free(bufEnvio);

   }
}

