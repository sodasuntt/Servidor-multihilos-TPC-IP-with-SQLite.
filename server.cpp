#include <iostream>
#include <cstring>        // Para strlen().
#include <sqlite3.h>      // Compatibilidad para SQLite.
#include <sys/socket.h>   // Estructura de Sockets.
#include <netinet/in.h>   // Estructura de Red.
#include <unistd.h>       // Funciones POSIX (close, read, write).
#include <thread>         // Creacion de hilos.
#include <mutex>          // Protege recursos.
#include <chrono>         // Para manipular segundos.
#include <iomanip>        // Setw para las tablas.
#include <sstream>        // Uso de ostringstream.
#include <vector>         // Guardar los hilos en vectores
using namespace std;

#define PORT 8080
mutex mutexProductos;
mutex mutexProveedores;
sqlite3 *db;

// Funcion que recolecta la tabla Productos.
string mostrarTablaProductos(){
    ostringstream dataProductos; 
    string sql = "SELECT * FROM Productos;";   // Hacemos la selecion de toda la lista Productos.
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    int numColumnas = sqlite3_column_count(stmt);
    const int anchoColumna = 20;
    string cabecera(41, '=');

    // Mostramos la cabecera.
    dataProductos << "\n" << cabecera << " TABLA: Productos " << cabecera << "\n";
    for(int i=0; i<numColumnas; i++){
        dataProductos << left << setw(anchoColumna) << sqlite3_column_name(stmt, i);
    }
    // Division de cabecera representada con -
    string division(anchoColumna * numColumnas, '-');
    dataProductos << "\n" << division << "\n";

    // Mostramos todos los campos de Productos
    while (sqlite3_step(stmt) == SQLITE_ROW){
        for (int i = 0; i < numColumnas; i++){
            dataProductos << left << setw(anchoColumna) << sqlite3_column_text(stmt, i);
        }
        dataProductos << "\n";
    }
    dataProductos << "\n";
    sqlite3_finalize(stmt);
    return dataProductos.str();
}

// La misma funcion pero para la tabla Proveedores.
string mostrarTablaProveedores(){
    ostringstream dataProveedores;
    string sql = "SELECT * FROM Proveedores;";   // Hacemos la selecion de toda la lista Proveedores.
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

    int numColumnas = sqlite3_column_count(stmt);
    const int anchoColumna = 13;
    string cabecera(16, '=');

    // Mostramos la cabecera.
    dataProveedores << "\n" << cabecera << " TABLA: Proveedores " << cabecera << "\n";
    for(int i=0; i<numColumnas; i++){
        dataProveedores << left << setw(anchoColumna) << sqlite3_column_name(stmt, i);
    }
    // Division de cabecera representada con -
    string division(anchoColumna * numColumnas, '-');
    dataProveedores << "\n" << division << "\n";

    // Mostramos todos los campos de Proveedores
    while (sqlite3_step(stmt) == SQLITE_ROW){
        for (int i = 0; i < numColumnas; i++){
            dataProveedores << left << setw(anchoColumna) << sqlite3_column_text(stmt, i);
        }
        dataProveedores << "\n";
    }
    dataProveedores << "\n";
    sqlite3_finalize(stmt);
    return dataProveedores.str();
}

// Funcion que se comunica con los clientes.
void comunicacionCliente(int cliente_socket, int clienteID){
    char buffer[1024];
    const char *instruccion = "\nPor favor ingresa: 'tablaProductos', 'tablaProveedores' o 'Finalizado' para cerrar la sesion: ";
    while(true){
        memset(buffer, 0, sizeof(buffer));
        send(cliente_socket, instruccion, strlen(instruccion), 0);
        read(cliente_socket, buffer, sizeof(buffer));

        if (strcmp(buffer, "tablaProductos") == 0){
            if (mutexProductos.try_lock()){
                string tablaProductos = mostrarTablaProductos();
                send(cliente_socket, tablaProductos.c_str(), tablaProductos.size(), 0);
                cout << "El cliente esta observando la Tabla Productos!\n";
                this_thread::sleep_for(chrono::seconds(3));
                mutexProductos.unlock();
            } else {
                const char* msg = "\nTabla Productos en uso, proba mas tarde.\n";
                write(cliente_socket, msg, strlen(msg));
                this_thread::sleep_for(chrono::seconds(1));
            }
        } else if (strcmp(buffer, "tablaProveedores") == 0){
            if (mutexProveedores.try_lock()){
                string tablaProveedores = mostrarTablaProveedores();
                send(cliente_socket, tablaProveedores.c_str(), tablaProveedores.size(), 0);
                cout << "El cliente esta observando la Tabla Proveedores!\n";
                this_thread::sleep_for(chrono::seconds(5));
                mutexProveedores.unlock();
            } else {
                const char* msg = "\nTabla Proveedores en uso, proba mas tarde.\n";
                write(cliente_socket, msg, strlen(msg));
                this_thread::sleep_for(chrono::seconds(1));
            }
        } else if (strcmp(buffer, "Finalizado") == 0){
            close(cliente_socket);
            cout << "\nEl cliente finalizo su sesion." << endl;
            break;
        } else {
            const char *desconocido = "\nComando desconocido, por favor intenta de nuevo \n";
            send(cliente_socket, desconocido, strlen(desconocido), 0);
            this_thread::sleep_for(chrono::seconds(1));
        }
    }
}

int main(){
    // Definimos el socket.
    int server_socket, cliente_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    int opt = 1;
    char buffer[1024] = {0};
    int num_cliente = 0;

    // Creamos el socket.
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // Lo hacemos reutilizable.

    address.sin_family = AF_INET; // Direccion IPv4.
    address.sin_addr.s_addr = INADDR_ANY; // Escucha en todas las interfaces.
    address.sin_port = htons(8080); // Le asignamos el puerto correspondiente.

    // Inicializamos la base de datos.
    int rc;
    rc = sqlite3_open("negocio.db", &db);

    // Mostramos si se pudo abrir correctamente la base de datos.
    if (rc){
        cout << "No se pudo abrir la base de datos.\n";
    } else {
        cout << "Se abrio correctamente la base de datos.\n";
    }

    // Habilitamos la escucha del server.
    bind(server_socket, (struct sockaddr *)&address, sizeof(address));
    listen(server_socket, 1);
    cout << "Esperando al cliente...\n";

    // Aceptamos maximo 2 clientes/hilos.
    vector<int>clienteSockets;
    vector<thread>hilos;
    for(int i=0; i<2; i++){
            cliente_socket = accept(server_socket, (struct sockaddr *)&address, &addrlen);
            cout << "Cliente "<<i+1<<" conectado!\n";
            cout << "Esperando instrucciones...\n";
            // Asignamos a cada hilo su tarea.
            hilos.emplace_back(thread(comunicacionCliente, cliente_socket, i+1));
    }

    // Esperamos que los hilos terminen.
    for (auto& hilo : hilos){
        if (hilo.joinable()){
            hilo.join();
        }
    }

    // Cerramos los sockets y la base de datos.
    close(server_socket);
    sqlite3_close(db);
    return 0;
}
