#include <iostream>
#include <string>
#include <arpa/inet.h>
#include <cstring>        // Para strlen() y memset.
#include <unistd.h>       // Funciones POSIX (close, read, write).
#include <netinet/in.h>   // Estructura de Red.
#include <sys/socket.h>   // Estructura de Sockets.
using namespace std;

#define PORT 8080

int main(){
    int cliente_socket;
    sockaddr_in servidor_addr{};
    char buffer[1024];

    // Creamos el socket y lo configuramos
    cliente_socket = socket(AF_INET, SOCK_STREAM, 0);
    servidor_addr.sin_family = AF_INET;
    servidor_addr.sin_port = htons(8080);

    // Nos conectamos al servidor.
    connect(cliente_socket, (struct sockaddr*)&servidor_addr, sizeof(servidor_addr));
    cout << "Conectado al servidor con exito!" << endl;

    // Se piden instrucciones.
    string instruccion;
    while(true){
        memset(buffer, 0, sizeof(buffer));
        recv(cliente_socket, buffer, 1024, 0);
        cout << buffer;
        getline(cin, instruccion);
        send(cliente_socket, instruccion.c_str(), instruccion.length(), 0);
        if (instruccion == "Finalizado"){
            cout << "Finalizaste tu sesion con el servidor!\n";
            close(cliente_socket);
            break;
        }
        memset(buffer, 0, sizeof(buffer));
        int servidor = read(cliente_socket, buffer, 1024);
        cout << buffer;
        }
    }