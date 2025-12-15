#include <iostream>
#include <chrono>
#include <cmath>
#include <MinimalSocket/udp/UdpSocket.h>
#include <vector>
#include <map>

using namespace std::chrono;
using namespace std;

double rad2deg(double rad){
    return rad*180.0/M_PI;
}

void sendMessage(string msg,
                 MinimalSocket::udp::Udp<true> &udp_socket,
                 MinimalSocket::Address const &server_udp){
    msg.push_back('\0');
    udp_socket.sendTo(msg, server_udp);
}

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

enum PlayMode {
    PM_NORMAL,
    PM_KICK_IN
};

PlayMode play_mode;
bool just_took_kick_in=false; // Bandera para indicar tiro libre

bool isDefender(int num){
    return (num == 2 || num == 3 || num == 4 || num == 5);
}
bool isMiddle(int num){
    return (num == 6 || num == 8 || num == 10);
}
bool isStriker(int num){
    return (num == 7 || num == 9 || num == 11);
}

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
    float dist_chg;  // velocidad radial
    float dir_chg;   // velocidad angular
    bool found;
};

// Hallamos el ángulo hacia cualquier bandera
FlagInfo getFlagInfo(string const &msg, string const &flag){
    FlagInfo res;
    res.dist = 0.0;
    res.dir = 0.0;
    res.dist_chg = 0.0;
    res.dir_chg = 0.0;
    res.found = false;

    size_t pos = msg.find(flag);
    if(pos == string::npos) return res;

    size_t offset = msg.find(')', pos);
    if(offset == string::npos) return res;

    float v1, v2, v3, v4;
    int result = 0;

    // BALÓN
    if(flag == "((b)"){
        result = sscanf(msg.c_str() + offset + 1,
                        "%f %f %f %f", &v1, &v2, &v3, &v4);
        if(result >= 4){
            res.dist = v1;
            res.dir = v2;
            res.dist_chg = v3;
            res.dir_chg = v4;
            res.found = true;
        }
    }
    // RESTO DE OBJETOS
    else{
        result = sscanf(msg.c_str() + offset + 1, "%f %f", &v1, &v2);
        if(result == 2){
            res.dist = v1;
            res.dir = v2;
            res.found = true;
        }
        else if(result == 1){
            res.dist = 1000.0;
            res.dir = v1;
            res.found = true;
        }
    }

    return res;
}

void centerOrient(string const &recv_msg_content, bool &orient,
                  MinimalSocket::udp::Udp<true> &udp_socket,
                  MinimalSocket::Address const &server_udp)
{
    int contador{0};
    // Nos interesa la dirección hacia el centro: center.dir
    FlagInfo center = getFlagInfo(recv_msg_content, "((f c)");
    if(!center.found && contador <= 3){
        // NO se encuentra el centro --> Giro 90º hasta verlo
        udp_socket.sendTo("(turn 90)", server_udp);
        contador++;
    }else{
        // SI se encuentra el centro --> Giro hacia él
        udp_socket.sendTo("(turn " + to_string(center.dir) + ")", server_udp);
        orient = false; // Para no volver a llamar a "centerOrient" desde el main
    }
}

float predictBallDir(const FlagInfo &ball){
    // Tiempo de predicción (~2 ciclos = 100ms)
    const float PRED_TIME = 0.10;

    // dir_chg es por ciclo (50ms)
    float angular_speed = ball.dir_chg / 0.05;

    float predicted_dir = ball.dir + angular_speed * PRED_TIME;

    // Normalizar
    while(predicted_dir > 180) predicted_dir -= 360;
    while(predicted_dir < -180) predicted_dir += 360;

    // Si el balón se aleja rápido, no persigas
    if(ball.dist_chg > 0.3)
        return ball.dir;

    return predicted_dir;
}


void parseSeeMessage(string const &recv_msg_content, Player const& p,
                     string target_goal, string team_name,
                     MinimalSocket::udp::Udp<true> &udp_socket,
                     MinimalSocket::Address const &server_udp,
                     map<int, pair<float, steady_clock::time_point>> &teammates_dist)
{
    // Se inicializa hace 10 segundos para que al empezar el partido puedas correr.
    // Al ser static, este valor se mantiene entre llamadas a la función.

    // BALÓN --> Bandera ((b)
    FlagInfo ball = getFlagInfo(recv_msg_content, "((b)");


    // ======================================================
    // =================== SAQUE DE BANDA ===================
    // ======================================================
    if(play_mode == PM_KICK_IN)
    {
        // Si no se ve el balón, girar para encontrarlo
        if(!ball.found){
            udp_socket.sendTo("(turn 30)", server_udp);
            return;
        }

        // Publico mi distancia al balón
        teammates_dist[p.num] = make_pair(ball.dist, steady_clock::now());
        udp_socket.sendTo("(say \"" + to_string(p.num) + " " + to_string(ball.dist) + "\")", server_udp);

        // Decidir jugador más cercano
        bool closest = true;
        auto now = steady_clock::now();

        for(const auto &it : teammates_dist){
            int num = it.first;
            float dist = it.second.first;
            auto t = it.second.second;

            if(num == p.num) continue;
            if(duration_cast<milliseconds>(now - t).count() > 500) continue;

            if(dist < ball.dist){
                closest = false;
                break;
            }
        }

        // ---------- EJECUTOR ----------
        if(closest){
            // Orientarse al balón
            if(abs(ball.dir) > 5){
                udp_socket.sendTo("(turn " + to_string(ball.dir) + ")", server_udp);
                return;
            }
            // Acercarse hasta estar muy cerca
            if(ball.dist > 1.2){
                udp_socket.sendTo("(dash 120)", server_udp);
                return;
            }

            // Ya estamos junto al balón → hay que PASAR al compañero más cercano
            string mate_prefix = "((p \"" + team_name + "\"";
            float best_dist = 1e9f;
            float best_dir = 0.0f;
            bool found_mate = false;

            // Escanear posibles compañeros por dorsal (1..11)
            for(int n = 1; n <= 11; ++n){
                if(n == p.num) continue; // no me paso a mí mismo
                string flag = mate_prefix + " " + to_string(n) + ")";
                FlagInfo mate = getFlagInfo(recv_msg_content, flag);
                if(mate.found && mate.dist < best_dist){
                    best_dist = mate.dist;
                    best_dir = mate.dir;
                    found_mate = true;
                }
            }

            if(found_mate){
                float power = best_dist * 2.5f;
                if(power > 100.0f) power = 100.0f;
                if(power < 30.0f) power = 30.0f;
                string kick_cmd = "(kick " + to_string(power) + " " + to_string(best_dir) + ")";
                sendMessage(kick_cmd, udp_socket, server_udp);
            }else{
                // Si no se ve ningún compañero, chutamos suave hacia delante
                sendMessage("(kick 30 0)", udp_socket, server_udp);
            }

            // El balón ya se ha puesto en juego → volvemos al modo normal
            play_mode = PM_NORMAL;
            just_took_kick_in = true;
            return;
        }

        // ---------- NO EJECUTORES ----------
        // Se quedan quietos (sin desplazarse), solo manteniendo orientación
        udp_socket.sendTo("(turn 0)", server_udp);
        return;
    }

    if(ball.found){
        // Balón a la vista --> Actualizo mi distancia hacia él en el mapa
        teammates_dist[p.num] = make_pair(ball.dist, steady_clock::now());
        // ======================================================
        // ===================== PORTERO ========================
        // ======================================================
        if (p.num == 1){
            // Determinar nuestra portería para saber dónde está nuestra posición segura
            string own_goal_flag = (p.side == 'l') ? "((g l)" : "((g r)";
            FlagInfo own_goal = getFlagInfo(recv_msg_content, own_goal_flag);

            // 1. Balón en zona inmediata (Atajar / Despejar)
            if (ball.dist < 1.5) {
                if (ball.dist < 0.5) {
                    // Despejar potentemente
                    udp_socket.sendTo("(kick 100 0)", server_udp);
                } else {
                    // Atajar
                    udp_socket.sendTo("(catch " + to_string(ball.dir) + ")", server_udp);
                }
                return;
            }

            // 2. Balón peligroso: PREDICCIÓN y movimiento a la intercepción
            if (ball.dist < 20.0 && ball.dist_chg < -0.1) {

                float target_dir = predictBallDir(ball);

                if (abs(target_dir) > 5.0) {
                    // Girar hacia la dirección predicha
                    udp_socket.sendTo("(turn " + to_string(target_dir) + ")", server_udp);
                    return;
                }

                if (ball.dist > 1.0) {
                    // Correr hacia la intercepción con potencia variable
                    float dash_power = 60 + abs(ball.dist_chg) * 50;
                    if (dash_power > 100) dash_power = 100;

                    udp_socket.sendTo("(dash " + to_string(dash_power) + ")", server_udp);
                    return;
                }

                // Bien posicionado
                udp_socket.sendTo("(turn 0)", server_udp);
                return;
            }

            // 3. Posicionamiento defensivo (balón lejos o sin amenaza)
            if (own_goal.found) {
                const float SAFE_DIST_FROM_GOAL = 3.0;

                // Demasiado lejos de la portería
                if (own_goal.dist > SAFE_DIST_FROM_GOAL + 1.0) {
                    if (abs(own_goal.dir) > 5.0) {
                        udp_socket.sendTo("(turn " + to_string(own_goal.dir) + ")", server_udp);
                    } else {
                        udp_socket.sendTo("(dash 50)", server_udp);
                    }
                    return;
                }
                // Demasiado dentro de la portería
                else if (own_goal.dist < SAFE_DIST_FROM_GOAL - 1.0) {
                    if (abs(own_goal.dir) > 5.0) {
                        udp_socket.sendTo("(turn " + to_string(own_goal.dir) + ")", server_udp);
                    } else {
                        udp_socket.sendTo("(dash -30)", server_udp);
                    }
                    return;
                }
                // Posición segura
                else {
                    udp_socket.sendTo("(turn " + to_string(ball.dir) + ")", server_udp);
                    return;
                }
            }

            // Ve el balón pero no su portería
            udp_socket.sendTo("(turn " + to_string(ball.dir) + ")", server_udp);
            return;
        }

        // ======================================================
        // =============== RESTO DE JUGADORES ===================
        // ======================================================
        else{

            if(ball.dist < 0.7){
                // ... (Lógica cuando tiene el balón en los pies) ...
                FlagInfo goal = getFlagInfo(recv_msg_content, target_goal);

                if(goal.found){
                    if(goal.dist < 25.0){
                        // Portería cerca --> CHUTO
                        udp_socket.sendTo("(kick 100 " + to_string(goal.dir)+ ")", server_udp);
                    }else{
                        udp_socket.sendTo("(kick 20 " + to_string(goal.dir)+ ")", server_udp);
                    }
                }else{
                    // NO ve la portería --> Uso el centro para orientarme
                    FlagInfo center = getFlagInfo(recv_msg_content, "((f c)");
                    if(center.found){
                        // Veo el centro --> Lo uso para orientarme
                        if (abs(center.dir) > 120.0){
                            // El centro está detrás: entre 120º y -120º --> Sigo hacia delante
                            sendMessage("(kick 20 0)", udp_socket, server_udp);
                        }else if (abs(center.dir) < 30.0){
                            // El centro está en la dirección de mi nariz --> Sigo hacia él
                            sendMessage("(kick 20 0)", udp_socket, server_udp);
                        }else{
                            // El centro está a un lado: aprox. 90º o -90º --> Giramos
                            sendMessage("(turn 30)", udp_socket, server_udp);
                        }
                    }else{
                        // No ve la portería NI el centro --> Busco un compañero
                        string mate_msg_3 = "((p \""+ team_name + "\" 3";
                        string mate_msg_5 = "((p \""+ team_name + "\" 5";
                        FlagInfo mate3 = getFlagInfo(recv_msg_content, mate_msg_3);
                        FlagInfo mate5 = getFlagInfo(recv_msg_content, mate_msg_5);
                        if(mate3.found){
                            float kick_power = mate3.dist*2.0; // Más fuerte si está más lejos
                            if(kick_power > 100){ kick_power = 100;}
                            if(kick_power < 40){kick_power = 40;}
                            string msg = "(kick " + to_string(kick_power) + " " + to_string(mate3.dir) + ")";
                            sendMessage(msg, udp_socket, server_udp);
                        }else if(mate5.found){
                            float kick_power = mate5.dist*2.0; // Más fuerte si está más lejos
                            if(kick_power > 100){ kick_power = 100;}
                            if(kick_power < 40){kick_power = 40;}
                            string msg = "(kick " + to_string(kick_power) + " " + to_string(mate5.dir) + ")";
                            sendMessage(msg, udp_socket, server_udp);
                        }else{
                            udp_socket.sendTo("(kick 15 90)", server_udp);
                        }
                    }
                }
            }else{
                // =============================================
                // SI NO tengo el balón en los pies
                // =============================================
                if(abs(ball.dir) > 10.0){
                    udp_socket.sendTo("(turn " + to_string(ball.dir) + ")", server_udp);
                }else{
                    // ==========================================================
                    // BLOQUE DONDE DECIDIMOS SI CORRER AL BALÓN
                    // ==========================================================
                    if(ball.dist < 20.0){
                        // -----------------------------------------------------------------
                        // APLICACIÓN DE LAS DELIMITACIONES POSICIONALES (TU CÓDIGO)
                        // -----------------------------------------------------------------
                        bool must_stop = false;

                        if(isDefender(p.num)){
                            FlagInfo center = getFlagInfo(recv_msg_content, "((f c)");
                            FlagInfo center_b = getFlagInfo(recv_msg_content, "((f c b)");
                            FlagInfo center_t = getFlagInfo(recv_msg_content, "((f c t)");
                            if((center.found && abs(center.dist) < 20) ||
                                (center_b.found && abs(center_b.dist) < 20) ||
                                (center_t.found && abs(center_t.dist) < 20)){
                                // Defensa cerca del centro (X=0)
                                must_stop = true;
                            }
                        }else if(isMiddle(p.num)){
                            FlagInfo goal = getFlagInfo(recv_msg_content, target_goal);
                            if(goal.found && abs(goal.dist) < 15){
                                // Medio cerca de la portería contraria (no avanzar más)
                                must_stop = true;
                            }
                        }else if(isStriker(p.num)){
                            string own_goal = (p.side == 'l') ? "((g l)" : "((g r)";
                            FlagInfo own = getFlagInfo(recv_msg_content, own_goal);
                            if(own.found && abs(own.dist) < 40){
                                // Delantero muy cerca de su propia portería
                                must_stop = true;
                            }
                        }

                        if (must_stop) {
                            udp_socket.sendTo("(turn 0)", server_udp);
                        }else{
                            udp_socket.sendTo("(dash 200)", server_udp);
                        }

                    }else{
                        // Balón lejos --> Habría que defender: acercarse al jugador contrario más cercano
                        udp_socket.sendTo("(turn 0)", server_udp);
                    }

                }
            }
        }
    }else{
        // NO vemos el balón --> giramos para buscarlo
        udp_socket.sendTo("(turn 45)", server_udp);
    }
}


void parseHearMessage(string const &recv_msg_content,bool &orient, Player const &p, bool &can_play, MinimalSocket::udp::Udp<true> &udp_socket,MinimalSocket::Address const &server_udp, map<int, pair<float, steady_clock::time_point>> &teammates_dist)
{

    if(contains(recv_msg_content, "referee")){
        // Procede del árbitro
        //cout << "Árbitro dice: " << recv_msg_content << endl;

        if(contains(recv_msg_content, "goal_") || contains(recv_msg_content, "kick_off_")
            || contains(recv_msg_content, "half_time")){
            sendInitialMoveMessage(p, udp_socket, server_udp); // Todos a sus posiciones iniciales
            can_play = false; // No se puede jugar
            orient = true; // Nos orientamos hacia el centro en el siguiente ciclo
            play_mode = PM_NORMAL;
            just_took_kick_in = false;
        }else if(contains(recv_msg_content, "play_on")){
            can_play = true; // Se puede jugar
            orient = false; // No es necesario orientarse al centro mientras se juega
            // Si estábamos en un saque de banda pero el árbitro dice play_on directamente, volvemos al modo normal
            if(play_mode == PM_KICK_IN){
                play_mode = PM_NORMAL;
            }
        }else if(contains(recv_msg_content, "drop_ball") || contains(recv_msg_content, "offside")){
            can_play = false; // NO se puede jugar
        }else if(contains(recv_msg_content, "kick_in_")){
            // El árbitro señala saque de banda
            if(
                (contains(recv_msg_content, "kick_in_l") && p.side=='l') ||
                (contains(recv_msg_content, "kick_in_r") && p.side=='r')
                ){
                cout << "Kick-in detectado" << endl;
                play_mode = PM_KICK_IN;
                just_took_kick_in = false;
                // Durante el saque de banda, el juego está pausado hasta que se ponga el balón en juego
                can_play = true;     // permitimos actuar al ejecutor
                orient = false;      // no hace falta orientarse al centro
            }else{
                // Es saque de banda del rival: este jugador no debe intentar sacar
                play_mode = PM_NORMAL;
            }
        }


    }else if(contains(recv_msg_content, "our")){
        // Procede de otro jugador de mi equipo: (hear 108 -16 our "9 7.400000")
        // Nos quedamos con el mensaje entre comillas
        size_t start_msg = recv_msg_content.find('\"');
        size_t end_msg = recv_msg_content.rfind('\"');
        if(start_msg != string::npos && end_msg != string::npos && end_msg > start_msg){
            string msg = recv_msg_content.substr(start_msg + 1, end_msg - start_msg - 1);
            int num{0}; // Dorsal del compañero
            float dist{0.0}; // Distancia al balón del compañero
            int read = sscanf(msg.c_str(), "%d %f", &num, &dist);
            if (read == 2){
                teammates_dist[num] = make_pair(dist, steady_clock::now()); // Guardamos la distancia de ese compañero
            }
        }
    }

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

    // Bandera para NO jugar cuando el árbitro pare el juego
    bool can_play = false;
    // Bandera para orientarse hacia el centro
    bool orient = true;
    // Definimos la portería objetivo: (g r) ó (g l)
    string target_goal = (p.side == 'l') ? "(g r)" : "(g l)";
    // Mapa para almacenar la distancia al balón de los compañeros
    map<int, pair<float, steady_clock::time_point>> teammates_dist;

    while(true){

        auto recv_msg = udp_socket.receive(message_max_size);
        auto recv_msg_content = recv_msg->received_message;

        if(contains(recv_msg_content, "(see"))
        {
            if(orient && !can_play){
                centerOrient(recv_msg_content, orient, udp_socket, server_udp);// Nos orientamos hacia el centro
                continue; //No envío más comandos hasta el siguiente ciclo del bucle
            }
            if(can_play){
                /*
                if(p.num == 9){
                    cout << "                    " << endl;
                    cout << recv_msg_content << endl;
                }*/
                // Puedo empezar a jugar una vez orientado
                parseSeeMessage(recv_msg_content, p, target_goal, team_name, udp_socket, server_udp, teammates_dist);
            }
        }
        else if(contains(recv_msg_content, "(hear"))
        {
            parseHearMessage(recv_msg_content, orient, p, can_play, udp_socket, server_udp, teammates_dist);
        }
    }

    return 0;
}
