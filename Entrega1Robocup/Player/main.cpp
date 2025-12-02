#include <iostream>

using namespace std;

#include <MinimalSocket/udp/UdpSocket.h>
#include <chrono>
#include <thread>
#include <vector>

struct player{
    int num;
    char side;
    void parseInit(string &os){

        size_t i = 6;
        side = os.at(i);
        i += 2;

        int espa = os.find(' ', i);
        string num_str = os.substr(i, espa - i);
        num = stoi(num_str);
    }


};

void moveToInitialPosition(player const &p,MinimalSocket::udp::Udp<true> &udp_socket,
                           MinimalSocket::Address const &server_udp)
{
    struct Posicion
    {
        int x, y;
    };
    vector<Posicion>
        posiciones = {{-50, 0},
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
    Posicion myPos= posiciones[p.num-1];

    auto move_cmd = "(move " + to_string(myPos.x) + " " + to_string(myPos.y) + ")";
    udp_socket.sendTo(move_cmd, server_udp);
}

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
    cout << "(init " + team_name + "(version 19))";

    udp_socket.sendTo("(init " + team_name + "(version 19))", other_recipient_udp);
    cout << "Init Message sent" << endl;

    std::size_t message_max_size = 1000;
    cout << "Waiting for a message" << endl;
    auto received_message = udp_socket.receive(message_max_size);
    std::string received_message_content = received_message->received_message;
    player p;
    p.parseInit(received_message_content);
    cout<<"Side: "<< p.side<<endl;
    cout<<"Dorsal: "<< p.num<<endl;

    // update upd port to provided by the other udp
    MinimalSocket::Address other_sender_udp = received_message->sender;
    MinimalSocket::Address server_udp = MinimalSocket::Address{"127.0.0.1", other_sender_udp.getPort()};
    cout<<"Puerto udp: "<<server_udp.getPort()<<endl;


    moveToInitialPosition(p,udp_socket, server_udp);

    while(1){
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}
