#include <sys/socket.h> 
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>     
#include <cstring>      // Para memset
#include <iostream>

#include <arpa/inet.h>

int DEFAULT_SERVER_PORT = 5000;
char* DEFAULT_SERVER_IP = "localhost";


const int QUIT = 299;
const int NEW_MSG = 300;
const int MSG_PRIV_O = 301;
const int WARNING_MOV = 302;
const int MAIL_MSG = 303;
const int SERVER_BUSY = 421;
const int NICK_INVALID = 433;
const int NOT_MEMBER = 442;
const int INVALID_CMD = 500;
const int INVALID_SINTAX = 501;
const int LINE_OVERFLOW = 502;
const int NOT_SINGUP = 530;
const int NICK_NOT_EXISTS = 550;
const int CHAN_NOT_EXISTS = 551;
const int INDEX_NOT_MSG = 552;

char CR = '\r';
char LF = '\n';

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
    // bzero((char *) &serv_addr, sizeof(serv_addr));
    memset(&serv_addr, 0, sizeof(serv_addr));

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

    // 512B de tamaño total de la line + 2B CRLF + 1B para null termination porsia las moscas
    char buffer[515];
    
    int op = 0;
    while (op != QUIT)
    {
        std::string g;
        memset(buffer, 0x00, sizeof(buffer));

        // si se usa .at() para recorrer el string no se toma en cuenta el null termination
        // osea, da un error, pero si se usa [] si se toma en cuenta, es decir, si puedo mostrar ese 0x00
        // lo otro es que si aqui ocurre un error, se cierra la conexion repentinamente con el server
        // y, gracias a dios, el server ya estaba preparado para eso

        std::getline(std::cin, g);

        // lenght() esta contando hasta null termination sin incluirlo, y empieza desde 1
        // (no como indice)
        
        // entonces la linea va a tener:
        //  max 4B    |    disponible 508B
        // el comando + sus argumentos (incluido texto) 
        // necesariamente nosotros debemos colocar el CRLF

        if(g.length() > 512){
            std::cerr << "ERROR: el tamaño maximo de la linea son 512B y te pasaste :P";
            close(fd_socket);
            return 1;
        }

        // esto no deberia dar core dumped primero porque el buffer es mas grande que g
        // y porque al usar [] podriamos gozar del colchon de null termination, po lo que sea
        int i = 0;
        while (i < g.length())
        {
            buffer[i] = g[i];
            i++;
        }

        // ---------------- CRLF ----------------------
        // i queda en la ultima posicion disponible
        buffer[i] = '\r';
        buffer[i + 1] = '\n';
        // ---------------- CRLF ----------------------


        
        size_t n = send(fd_socket, buffer, sizeof(buffer), 0);
        if (n < 0){
            std::cerr << "ERROR: no logro enviar: " << errno << "\n";
            close(fd_socket);    
            return 1;
        }

        
        memset(buffer, 0x00, sizeof(buffer));
        n = recv(fd_socket, buffer, sizeof(buffer), 0);
        if (n < 0){
            std::cerr << "ERROR: no logro obtener acuse de recibido: " << errno << "\n";
            close(fd_socket);
            return 1;
        }

        // el server siempre responde con un codigo de clase, da igual si es multi linea o no
        op = 0;
        op += (buffer[0] - 48) * 100;
        op += (buffer[1] - 48) * 10;
        op += (buffer[2] - 48);

        

        printf("%s\n", buffer);

    }

    
    close(fd_socket);


    return 0;

}