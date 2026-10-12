#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h>     
#include <cstring>
#include <iostream>
#include <thread>
#include <mutex>

int DEFAULT_CHAT_PORT = 5000;
int DEFAULT_HTTP_PORT = 8080;
std::string DEFAULT_MAIL = "./buzones";

std::string QUIT = "quit";
std::string NICK = "nick";
std::string JOIN = "join";
std::string PART = "part";
std::string MSG = "msg";
std::string PRIV = "priv";
std::string CHANS = "chans";
std::string WHO = "who";
std::string STAT = "stat";
std::string LIST = "list";
std::string RETR = "retr";
std::string DELE = "dele";

std::string C_QUIT = "299";
std::string C_NEW_MSG = "300";
std::string C_MSG_PRIV_O = "301";
std::string C_WARNING_MOV = "302";
std::string C_MAIL_MSG = "303";
std::string C_SERVER_BUSY = "421";
std::string C_NICK_INVALID = "433";
std::string C_NOT_MEMBER = "442";
std::string C_INVALID_CMD = "500";
std::string C_INVALID_SINTAX = "501";
std::string C_CRLF_EXC = "502";
std::string C_NOT_SINGUP = "530";
std::string C_NICK_NOT_EXISTS = "550";
std::string C_CHAN_NOT_EXISTS = "551";
std::string C_INDEX_NOT_MSG = "552";
char CR = '\r';
char LF = '\n';


struct chat_http_mail
{
    int chat_port;
    int http_port;
    std::string mail;
    int fail;
};

struct reply_status
{
    std::string reply;
    bool status;
};

std::mutex policia;
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
    // bzero((char*) &serv_addr, sizeof(serv_addr) );
    memset(&serv_addr, 0x00, sizeof(serv_addr));

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

bool Valid_MSG(char* buffer){

    bool valid_msg = false;
    for(int i; i < 515; i++){
        // si i es 514 (el ultimo indice) no se cumple
        if (buffer[i] == CR && i < 515 - 1){
            if(buffer[i + 1] == LF){
                // mensaje valido
                valid_msg = true;
                break;
            }
            else{
                // mensaje invalido
                valid_msg = false;
                break;
            }
        } 
        else{
            // mensaje invalido
            valid_msg = false;
            break;
        }
    }    

    return valid_msg;
}

std::string Command_Handle(bool valid_msg, char* buffer){
    std::string comando;
    std::string replystr = "";
    bool status;

    if(valid_msg){
        int j = 0;
        while (buffer[j] != ' ' && buffer[j] != CR)
        {
            comando += buffer[j];
            j++;
        }
        {
            std::lock_guard<std::mutex> lock(policia);
            std::cout << "comando recibido: " << comando << "\n";
        }

        if(comando == QUIT){
            replystr += C_QUIT +  " " + "adiós" + CR + LF;
            status = false;
        }
        else{
            replystr += C_INVALID_CMD + " " + "comando invalido" + CR + LF;
        }
    }
    else{
        replystr += C_CRLF_EXC + " " + "linea muy larga y/o sin CRLF" + CR + LF;
    }
}



void Manage_Client(int fd_cli_socket){
    char buffer[515];
    bool status = true;

    while (status)
    {
        memset(buffer, 0x00, sizeof(buffer));
        std::string comando;
        std::string replystr = "";

        size_t n = recv(fd_cli_socket, buffer, 515, 0);
        if (n < 0){
            std::cerr << "ERROR: hubo un problema al recibir el mensaje: " << errno << "\n";
            break;
        }
        
        if(n == 0){
            // el cliente cerro la coneccion TCP sin hacer el quit
            // esta nomenclatura cheverisima nojoda
            {
                std::lock_guard<std::mutex> lock(policia);
                std::cout << "el cliente cerro la conexion" << "\n";
            }
            break;
        }
        

        bool valid_msg = Valid_MSG(buffer);


        // antes de entrar aqui necesariamente debemos haber cumplido que el mensaje tenga CRLF
        
        if(valid_msg){
            int j = 0;
            while (buffer[j] != ' ' && buffer[j] != CR)
            {
                comando += buffer[j];
                j++;
            }
            {
                std::lock_guard<std::mutex> lock(policia);
                std::cout << "comando recibido: " << comando << "\n";
            }

            if(comando == QUIT){
                replystr += C_QUIT +  " " + "adiós" + CR + LF;
                status = false;
            }
            else{
                replystr += C_INVALID_CMD + " " + "comando invalido" + CR + LF;
            }
        }
        else{
            replystr += C_CRLF_EXC + " " + "linea muy larga y/o sin CRLF" + CR + LF;
        }
        
        
        // aqui suponemos que ya hice lo que tenia que hacer con el mensaje del cliente
        memset(buffer, 0x00, sizeof(buffer));
        std::strcpy(buffer, replystr.c_str());

        n = send(fd_cli_socket, buffer, sizeof(buffer), 0);
        if (n < 0){
            std::cerr << "ERROR: hubo un problema al hacer acuse de recibido: " << errno << "\n";
            close(fd_cli_socket);
            break;
        }
    }
    
    {
        std::lock_guard<std::mutex> lock(policia);
        
        std::cout << "terminando conexion exitosamente con: " << fd_cli_socket << "\n";
    }
    close(fd_cli_socket);
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

    // el estandar dice que el tamano de la cola debe ser 5, y que si ponemos
    // un socket valido, el codigo no puede fallar, tonces no hace falta verificar
    listen(fd_socket, 5);
    
    while (true)
    {
        
        sockaddr_in client_addr;
        // el tamaño en bytes del addr del cliente , en el estandar lo definen siempre
        // como un int, pero no fnciona asi, hay otros sitios que directamente le ponen el tipo de 
        // socklen_t pero el mimo dice que es un unsigned int, y hay sitios que ponen el u_int, tal
        // parece que son intercambiables
        socklen_t client_len = sizeof(client_addr);


        int fd_cli_socket = accept(fd_socket, (sockaddr*)&client_addr, &client_len);

        {
            std::lock_guard<std::mutex> lock(policia);
            std::cout << "aceptamos un cliente nuevo: " << fd_cli_socket << "\n";
        }


        std::thread client_handler(Manage_Client, fd_cli_socket);
        
        client_handler.detach();


    }
    close(fd_socket);


    return 0;
}