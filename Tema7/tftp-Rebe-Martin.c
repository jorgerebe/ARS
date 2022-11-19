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
#define MAX_FILE_SIZE 100

#define OPCODE_RRQ   1
#define OPCODE_WRQ   2
#define OPCODE_DATA  3
#define OPCODE_ACK   4
#define OPCODE_ERROR 5

#define NOMBRE_SERVICIO "tftp"
#define MODO "octet"

void lectura(int sockfd, struct sockaddr_in addr_server, char* fileName, char ACK[], bool verbose);
void escritura(int sockfd, struct sockaddr_in addr_server, char*fileName, bool verbose);
void sendACK(int sockfd, struct sockaddr_in addr_server, char ACK[], short int block);

char* fileName;

int main(int argc, char* argv[]){

   int sockfd;
   bool verbose = false;
   struct sockaddr_in addr_server;
   char* fileName;

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
    *	comprobando que las opciones utilizadas son correctas (solo opcion -p)
    *
    *	Si se utiliza alguna otra, se muestra el error y se sale.
    */


   while((opt = getopt(argc, argv, "rwv")) != -1){
      switch(opt){
         case 'r':
            fileName = argv[optind];
            opcode = OPCODE_RRQ;
            break;
         case 'w':
            fileName = argv[optind];
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
    */

   request = (char*)malloc(BUF_SIZE*sizeof(char));
   *request = '\0';

   request[0] = (unsigned char)(opcode/256);
   request[1] = (unsigned char)(opcode%256);

   strcpy(request+2, fileName);
   strcpy(request+2+strlen(fileName)+1, MODO);



   /**
    *	Se envia un datagrama con una cadena arbitraria al puerto y servidor correspondiente
    */

   int envio = sendto(sockfd, request, BUF_SIZE, 0, (struct sockaddr*)&addr_server, sizeof(addr_server));

   if(envio == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
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
   }



   /*
    *	Se recibe la respuesta del servidor y se imprime por pantalla
    */

     


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

void sendACK(int sockfd, struct sockaddr_in addr_server, char ACK[], short int block){

   ACK[2] = (unsigned char)(block/256);
   ACK[3] = (unsigned char)(block%256);
#if DEBUG
   printf ("ACK[0] es %d\n", ACK[0]);
   printf ("ACK[1] es %d\n", ACK[1]);
   printf ("ACK[2] es %d\n", ACK[2]);
   printf ("ACK[3] es %d\n", ACK[3]);
#endif
   if(sendto(sockfd, ACK, 4, 0, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1){
      perror("sendto()");
      exit(EXIT_FAILURE);
   }
}


void lectura(int sockfd, struct sockaddr_in addr_server, char* fileName, char ACK[], bool verbose){

   FILE* file = fopen(fileName, "w");

   socklen_t addrlen = sizeof(struct sockaddr);

   char* bufRespuesta = NULL;
   bufRespuesta = (char*)malloc(BUF_SIZE*sizeof(char));

   int respuesta;
   short int opcode_respuesta;
   short block = -1;

 do{
      free(bufRespuesta);
      bufRespuesta = (char*)malloc(BUF_SIZE*sizeof(char));
      *bufRespuesta = '\0';

      respuesta = recvfrom(sockfd, bufRespuesta, BUF_SIZE, 0, (struct sockaddr*)&addr_server, &addrlen);

      if(respuesta == -1){
         perror("recvfrom()");
         exit(EXIT_FAILURE);
      }

#if DEBUG
      printf("Size respuesta:- %d\n", respuesta-4);
#endif

      opcode_respuesta = (unsigned char)bufRespuesta[0]*256 + (unsigned char)bufRespuesta[1];
      printf("OPCODE: %d\n", opcode_respuesta);

      switch(opcode_respuesta)
      {
         case OPCODE_DATA:
            block = (unsigned char)bufRespuesta[2]*256 + (unsigned char)bufRespuesta[3];
   	    printf ("RESPUESTA[2]: %d\tRESPUESTA[3]: %d\n", (unsigned char)bufRespuesta[2], (unsigned char)bufRespuesta[3]);
            break;
         case OPCODE_ERROR:
            fprintf(stderr, "%s\n", bufRespuesta+4);
            exit(EXIT_FAILURE);
      }

      if(verbose){
         printf("Recibido bloque del servidor tftp\n");
         printf("Es el bloque con codigo %d\n", block);
      }

      fwrite(bufRespuesta+4, 1, respuesta-4, file);

      sendACK(sockfd, addr_server, ACK, block);

      printf("\nSize respuesta: %d\n", respuesta-4);
            
   } while((respuesta-4) == 512);

   fclose(file); 
}

void escritura(int sockfd, struct sockaddr_in addr_server, char* fileName, bool verbose){
   
   FILE* file = fopen(fileName, "r");

   socklen_t addr_len = sizeof(struct sockaddr);

   char bufRespuesta[BUF_SIZE];

   char* bufEnvio;

   int bytesSent = -1;

   int respuesta;
   short int opcode_respuesta;
   short block = 0;
   short blockACK = -1;

   while(true){
      bufEnvio = (char*)malloc(BUF_SIZE*sizeof(char));

      respuesta = recvfrom(sockfd, bufRespuesta, BUF_SIZE, 0, (struct sockaddr*)&addr_server, &addr_len);

      if(respuesta == -1){
         perror("recvfrom()");
         exit(EXIT_FAILURE);
      }

      opcode_respuesta = (unsigned char)bufRespuesta[0]*256 + (unsigned char)bufRespuesta[1];

      switch(opcode_respuesta)
      {
         case OPCODE_ACK:
            blockACK = (unsigned char)bufRespuesta[2]*256 + (unsigned char)bufRespuesta[3];
            if(blockACK == block){
               if((bytesSent > -1) && (bytesSent < 512)){
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

      bufEnvio[0] = (unsigned char)(OPCODE_DATA/256);
      bufEnvio[1] = (unsigned char)(OPCODE_DATA%256);
      bufEnvio[2] = (unsigned char)(block/256);
      bufEnvio[3] = (unsigned char)(block%256);

      bytesSent = fread(bufEnvio+4, 1, 512, file);
      if(sendto(sockfd, bufEnvio, bytesSent+4, 0, (struct sockaddr*)&addr_server, sizeof(addr_server)) == -1){
         perror("sendto()");
         exit(EXIT_FAILURE);
      }

      free(bufEnvio);


   }

}
