#include <iostream>
#include <chrono>

using namespace std::chrono;
using namespace std;

#include <MinimalSocket/udp/UdpSocket.h>
#include <thread>
#include <vector>

struct Player{
    char side; // 'l' or 'r'
    int num;   // [1-11]
    void parseInit(string msg){
        // Estructura del mensaje: (init l 2 playmode)
        side = msg.at(6);
        int pos_before_playMode = msg.find(' ',8);
        num = stoi(msg.substr(8, pos_before_playMode));
    }
};

void sendInitialMoveMessage(Player const & p,
                            MinimalSocket::udp::Udp<true> &udp_socket,
                            MinimalSocket::Address const &server_udp)
{
    struct Posicion
    {
        int x;
        int y;
    };

    vector<Posicion> posiciones = {{-50, 0},
                                   {-40, -10},
                                   {-35, -28},
                                   {-40, 10},
                                   {-35, 28},
                                   {-25, -11},
                                   {-8, 20},
                                   {-25, 11},
                                   {-5, 0},
                                   {-15, 0},
                                   {-8, -20}};
    Posicion myPos = posiciones.at(p.num - 1);

    // (move X Y)
    auto moveCommand = "(move " + to_string(myPos.x) + " " + to_string(myPos.y) + ")";
    udp_socket.sendTo(moveCommand, server_udp);
}


bool contains(string msg, string target){
    return msg.find(target) != string::npos;
}

struct FlagInfo{
    float dist; // Número real positivo
    float dir; // [-180º, 180º]
    bool found;
};

// Hallamos el ángulo hacia cualquier bandera
FlagInfo getFlagInfo(string msg, string flag){
    FlagInfo res; // Estructura a devolver
    // Ejemplos de flag: ((b) 10.5 5.0 ...) , ((f t r 10) 16.1-7 0 0) ,  ((g r) 69.4 32), ...
    size_t pos = msg.find(flag);
    if(pos != string::npos){ // Comprobamos si encuentra la bandera
        float value1; // Número real positivo
        float value2; // [-180º, 180º]
        // Calculamos offset necesario para situarnos en el primer valor numérico
        // Ej. ((b) 10.5 5.0 ...) --> offset = 4
        int offset = flag.length();
        // Leemos los valores de distancia y dirección
        int result = sscanf(msg.c_str() + pos + offset, "%f %f", &value1, &value2);
        // Si la bandera sólo tiene un valor numérico asumimos que es el ángulo
        if(result == 2){
            // Caso usual: ((flag) dist dir)
            res.dist = value1;
            res.dir = value2;
            res.found = true;
        }else if(result == 1){
            // Otro caso: ((flag) dir)
            res.dist = 1000.0; // Le damos un valor grande
            res.dir = value1; // La dirección será el unico valor leído
            res.found = true;
        }
    }else{
        res.dist = 0.0; // Le damos un valor grande
        res.dir = 0.0; // La dirección será el unico valor leído
        res.found = false;
    }
    return res;
}

// main with two args
int main(int argc, char *argv[])
{
    // check if the number of arguments is correct
    if (argc != 3)
    {
        cout << "Usage: " << argv[0] << " <team-name> <this-port>" << endl;
        return 1;
    }

    // get the team name and the port
    string team_name = argv[1];
    MinimalSocket::Port this_socket_port = std::stoi(argv[2]);

    cout << "Creating a UDP socket" << endl;

    MinimalSocket::udp::Udp<true> udp_socket(this_socket_port, MinimalSocket::AddressFamily::IP_V6);

    cout << "Socket created" << endl;

    bool success = udp_socket.open();

    if (!success)
    {
        cout << "Error opening socket" << endl;
        return 1;
    }

    MinimalSocket::Address other_recipient_udp = MinimalSocket::Address{"127.0.0.1", 6000};
    cout << "(init " + team_name + "(version 19))" << endl;

    udp_socket.sendTo("(init " + team_name + "(version 19))", other_recipient_udp);
    cout << "Init Message sent" << endl;

    std::size_t message_max_size = 1000;
    cout << "Waiting for a message" << endl;
    auto received_message = udp_socket.receive(message_max_size);
    std::string received_message_content = received_message->received_message;

    std::cout << "Received message: " << received_message_content << std::endl;

    // update upd port to provided by the other udp
    MinimalSocket::Address other_sender_udp = received_message->sender;
    MinimalSocket::Address server_udp = MinimalSocket::Address{"127.0.0.1", other_sender_udp.getPort()};

    std::cout << "Server UDP Port: " << server_udp.getPort() << std::endl;

    Player p;
    p.parseInit(received_message_content);
    cout << "Campo: " << p.side << endl;
    cout << "Número: " << p.num << endl;

    sendInitialMoveMessage(p, udp_socket, server_udp);

    // Definimos la portería objetivo: (g r) ó (g l)
    string target_goal = (p.side == 'l') ? "(g r)" : "(g l)";
    // Bandera para NO jugar cuando el árbitro pare el juego
    bool can_play = false;

    while(true){

        auto recv_msg = udp_socket.receive(message_max_size);
        auto recv_msg_content = recv_msg->received_message;

        if(contains(recv_msg_content, "(see") && can_play){
            FlagInfo ball = getFlagInfo(recv_msg_content, "((b)");

            if(ball.found){
                // Vemos el balón --> Tomamos decisión según su distancia

                if(ball.dist < 0.7){
                    // Tengo el balón en los pies --> Tomamos decisión según bandera a la vista
                    FlagInfo goal = getFlagInfo(recv_msg_content, target_goal);

                    if(goal.found){
                        // Veo la portería contraria --> Chuto en dirección a la portería
                        udp_socket.sendTo("(kick 100 " + to_string(goal.dir) + ")", server_udp);
                    }else{
                        // NO veo la portería contraria --> Busco el centro
                        FlagInfo center = getFlagInfo(recv_msg_content, "((f c)");
                        if(center.found){
                            // Veo el centro --> Lo uso para orientarme

                            if (abs(center.dir) > 120.0){
                                // El centro está detrás: entre 120º y -120º --> Sigo hacia delante
                                udp_socket.sendTo("(kick 20 0)", server_udp);
                            }else if (abs(center.dir) < 30.0){
                                // El centro está en la dirección de mi nariz --> Sigo hacia él
                                udp_socket.sendTo("(kick 20 0)", server_udp);
                            }else{
                                // El centro está a un lado: aprox. 90º o -90º --> Giramos
                                udp_socket.sendTo("(turn 30)", server_udp);
                            }
                        }else{
                            // NO veo la portería NI el centro --> Giro para ver algo
                            udp_socket.sendTo("(turn 60)", server_udp);
                        }
                    }
                }else{
                    // NO Tengo el balón en los pies -->
                    if(abs(ball.dir) > 10.0){
                        // El balón NO está en dirección de mi nariz --> Giro hacia él
                        udp_socket.sendTo("(turn " + to_string(ball.dir) + ")", server_udp);
                    }else{
                        // El balón SI está en dirección de mi nariz --> Corro hacia él
                        udp_socket.sendTo("(dash 90)", server_udp);
                    }
                }
            }else{
                // NO vemos el balón --> giramos para buscar
                udp_socket.sendTo("(turn 30)", server_udp);
            }
        }else if(contains(recv_msg_content, "(sense_body")){
            //Información de velocidad, resistencia, etc

        }else if(contains(recv_msg_content, "(hear")){
            // Posibles origenes: árbitro, entrenadores u otros jugadores
            if(contains(recv_msg_content, "referee")){
                // Procede del árbitro
                cout << "Árbitro dice: " << recv_msg_content << endl;
                if(contains(recv_msg_content, "goal_")){
                    sendInitialMoveMessage(p, udp_socket, server_udp); // Todos a sus posiciones iniciales
                    can_play = false;
                }else if(contains(recv_msg_content, "play_on")){
                    can_play = true;
                }
            }
        }
    }

    return 0;
}
