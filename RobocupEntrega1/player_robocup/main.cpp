#include <iostream>

using namespace std;

#include <MinimalSocket/udp/UdpSocket.h>
#include <chrono>
#include <thread>


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

    while(true){
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }


    return 0;
}

