#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>     
#include <cstring>
#include <iostream>

int DEFAULT_CHAT_PORT = 5000;
int DEFAULT_HTTP_PORT = 8080;
std::string DEFAULT_MAIL = "./buzones";

struct chat_http_mail
{
    int chat_port;
    int http_port;
    std::string mail;
    int fail;
};

// simplemente toma el input de la llamada del programa,
// verifica si es correcto y devuelve un struct con los dos puertos
// y el directorio para el buzon
chat_http_mail CatchInput(int argc, char* argv[]){
    int c = -1;
    int h = -1;
    int b = -1;

    int chat_port;
    int http_port;
    std::string mail;
    int fail = 0;

    chat_http_mail chm;
    

    for(int i = 1; i < argc; i++){
        std::string flag = argv[i];

        if(flag == "--chat" || flag == "-c"){
            c = i + 1;
            i++;
        }
        else if(flag == "--http" || flag == "-h"){
            h = i + 1;
            i++;
        }
        else if(flag == "--buzones" || flag == "-b"){
            b = i + 1;
            i++;
        }
        else{
            std::cout << "ERROR: modo de uso: ./server \nbanderas:\n-c <puerto> | --chat <puerto>\t\t\t puerto para el chat\n"
            << "-h <puerto> | --http <puerto>\t\t\t puerto para el servicio http\n" 
            << "-b <directorio> | --buzones <directorio>\t directorio donde se coloca el buzon, se crea si no existe\n" << "\n";
            fail = 1;
        }
        
    }

    if (c > 0)
    {
        chat_port = atoi(argv[c]);
    }
    else{
        chat_port = DEFAULT_CHAT_PORT;
    }
    if (h > 0)
    {
        http_port = atoi(argv[h]);
    }
    else{
        http_port = DEFAULT_HTTP_PORT;
    }
    if (b > 0)
    {
        mail = argv[b];
    }
    else{
        mail = DEFAULT_MAIL;
    }

    chm.chat_port = chat_port;
    chm.http_port = http_port;
    chm.mail = mail;
    chm.fail = fail;

    return chm;
}


int Bind_Serv(int chat_port){

    // socket de escucha
    int fd_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(fd_socket < 0){ 
        // algo salio mal
        std::cerr << "ERROR: no se pudo crear el socket: " << errno << "\n";
        return -1;
    }

    sockaddr_in serv_addr;

    // lo hacemos todo cero pa que este limpio
    bzero((char*) &serv_addr, sizeof(serv_addr) );

    // y aqui le metemos todo lo que necesita
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(chat_port); // casteamos al endianosmo correcto, nojoda

    // reclamamos el puerto escogido como el puerto donde escucha el server
    if (bind(fd_socket, (sockaddr*)&serv_addr, sizeof(serv_addr)) < 0){
        
        std::cerr << "no se puedo hacer el bind: " << errno << "\n";
        return -1;
    }

    return fd_socket;
}


int main(int argc, char* argv[]){

    // ----- capturamos input ---------
    chat_http_mail input = CatchInput(argc, argv);

    int chat_port = input.chat_port;
    int http_port = input.http_port;
    std::string mail = input.mail;
    int fail = input.fail;

    if(fail == 1){
        return 1;
    }
    // ----- capturamos input ---------
    


    //______________________________________________
    //-----------BINDEO DEL SERVIDOR----------------
    //----------------------------------------------
    int fd_socket = Bind_Serv(chat_port);
    if(fd_socket < 0){ 
        return 1;
    }
    //----------------------------------------------
    //-----------BINDEO DEL SERVIDOR----------------
    //______________________________________________

    sockaddr_in client_addr;
    // el estandar dice que el tamano de la cola debe ser 5, y que si ponemos
    // un socket valido, el codigo no puede fallar, tonces no hace falta verificar
    listen(fd_socket, 5);

    // el tamaño en bytes del addr del cliente , en el estandar lo definen siempre
    // como un int, pero no fnciona asi, hay otros sitios que directamente le ponen el tipo de 
    // socklen_t pero el mimo dice que es un unsigned int, y hay sitios que ponen el u_int, tal
    // parece que son intercambiables
    socklen_t client_len = sizeof(client_addr);


    int fd_cli_socket = accept(fd_socket, (sockaddr*)&client_addr, &client_len);

    
    char buffer[512] = {0};

    size_t n = recv(fd_cli_socket, buffer, 512, 0);
    if (n < 0){
        std::cerr << "ERROR: hubo un problema al recibir el mensaje: " << errno << "\n";
    }
    printf("el buf %s", buffer);

    

    n = send(fd_cli_socket, "recibido", 9, 0);
    if (n < 0){
        std::cerr << "ERROR: hubo un problema al hacer acuse de recibido: " << errno << "\n";
    }

    close(fd_cli_socket);
    close(fd_socket);


    return 0;
}