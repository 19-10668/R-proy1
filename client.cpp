#include <sys/socket.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>     
#include <cstring>      // Para memset
#include <iostream>

#include <arpa/inet.h>

int DEFAULT_SERVER_PORT = 5000;
char* DEFAULT_SERVER_IP = "localhost";

struct ip_port
{
    int server_port;
    char* server_ip;
    int fail;
};

// simplemente toma el input de la llamada del programa,
// verifica si es correcto y devuelve un struct con el ip y el puerto
// especificados para el server
ip_port CatchInput(int argc, char* argv[]){

    int server_port;
    char* server_ip;
    int fail = 0;

    ip_port result;

    // para elegir el puerto podemos hacer que los argumentos sean <ip> <puerto> pero opcional
    if(argc == 1){
        //iniciamos el cliente por defecto en localhost en el puerto 5000
        server_port = DEFAULT_SERVER_PORT;
        server_ip = DEFAULT_SERVER_IP;
    }
    else if(argc == 3){
        // este si se especifica otro ip puerto
        
        server_port = atoi(argv[2]);
        server_ip = argv[1];
    }
    else{
        // y por si a caso se pone algo mas
        std::cout << "ERROR: Modo de uso: ./client [<ip> <port>] \n--- lo que esta entre [] es opcional" << "\n";
        fail = 1;
    }

    result.server_port = server_port;
    result.server_ip = server_ip;
    result.fail = fail;

    return result;
}


int Connect_C2S(char * ip, int port){

    // AF_INET es el protocolo IPv4
    // SOCK_STREAM es que vamos a usar TCP para comunicarnos
    // y el 0 no se que es pero segun es que lo que sea que se 
    // deba escojer se hace automatico, y es el estandar tonces asi se quedo
    int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_socket < 0){ 
        // algo salio mal
        std::cerr << "no se pudo crear el socket: " << strerror(errno) << "\n";
        return -1;
    }

    hostent *server;
    sockaddr_in serv_addr;
    bzero((char *) &serv_addr, sizeof(serv_addr));

    // esta parte es lo mismo que en server
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    // pero aqui viene lo del host:
    // queremos poder usar nombres ed host y no solo una ip cruda
    // tonces pormos usar este struct "hostent" para guarada alli la ip de un host
    // y esa ip la sacamos de usa esta funcion gethostname
    server = gethostbyname(ip);
    if(server == NULL){
        std::cerr << "ERROR: no se pudo obtener el host: " << strerror(errno) << "\n";
        return -1;
    }


    // esta funcion bcopy copia como strcpy, creo :P
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    // a partir de aqui ya tenemos el serv_addr listo pa usarse

    if (connect(fd_socket, (sockaddr*)&serv_addr,sizeof(serv_addr)) < 0){
        std::cerr << "ERROR: fallo al conectar: " << strerror(errno) << "\n";
        return -1;
    }

    return fd_socket;
}

int main(int argc, char* argv[]){

    // ----- capturamos input ---------
    int server_port;
    char* server_ip;
    int fail;

    ip_port input = CatchInput(argc, argv);

    server_port = input.server_port;
    server_ip = input.server_ip;
    fail = input.fail;

    if(fail == 1){
        return 1;
    }
    // ----- capturamos input ---------
    
    
    //______________________________________________
    //-----------CONEXION CLIENTE SERVIDOR----------
    //----------------------------------------------
    int fd_socket = Connect_C2S(server_ip, server_port);
    if(fd_socket < 0){
        return 1;
    }
    //----------------------------------------------
    //-----------CONEXION CLIENTE SERVIDOR----------
    //______________________________________________


    char buffer[512] = {0};
    buffer[0] = 'H';
    buffer[1] = 'E';
    buffer[2] = 'L';
    buffer[3] = 'L';
    buffer[4] = 'O';
    
    size_t n = send(fd_socket, buffer, 512, 0);
    if (n < 0){
        std::cerr << "ERROR: no logro enviar: " << errno << "\n";
        return 1;
    }

    bzero(buffer, 512);
    n = recv(fd_socket, buffer, 512, 0);
    if (n < 0){
        std::cerr << "ERROR: no logro obtener acuse de recivido: " << errno << "\n";
        return 1;
    }

    printf("mensaje: %s", buffer);

    close(fd_socket);


    return 0;

}