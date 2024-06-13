#include <iostream>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <vector>
#include <tclap/CmdLine.h>

const char* POEMS[] = {
        // A Poem from John Jenry Newmann
    "MAN is permitted much",
    "To scan and learn", 
    "In Nature’s frame;",
    "Till he well-nigh can tame",
    "Brute mischiefs, and can touch",
    "Invisible things, and turn",
    "All warring ills to purposes of good.", 
    "Thus, as a god below,", 
    "He can control,", 
    "And harmonize, what seems amiss to flow", 
    "As sever’d from the whole", 
    "And dimly understood.",
    "But o’er the elements", 
    "One Hand alone,", 
    "One Hand has sway.",
    "What influence day by day",
    "In straiter belt prevents", 
    "The impious Ocean, thrown", 
    "Alternate o’er the ever-sounding shore?", 
    "Or who has eye to trace", 
    "How the Plague came?", 
    "Forerun the doublings of the Tempest’s race?",
    "Or the Air’s weight and flame",
    "On a set scale explore?",
    "Thus God has will’d",
    "That man, when fully skill’d,", 
    "Still gropes in twilight dim;",
    "Encompass’d all his hours",
    "By fearfullest powers",
    "Inflexible to him.",
    "That so he may discern",
    "His feebleness,",
    "And e’en for earth’s success",
    "To Him in wisdom turn,",
    "Who holds for us the keys of either home,",
    "Earth and the world to come.",
    // A Poem from Shakespeare
    "Round about the couldron go:", 
    "In the poisones entrails throw.",
    "Toad,that under cold stone", 
    "Days and nights has thirty-one", 
    "Sweated venom sleeping got,", 
    "Boil thou first in the charmed pot.",
    "Double,double toil and trouble;", 
    "Fire burn and cauldron bubble.",
    "Fillet of a fenny snake,",
    "In the cauldron boil and bake;", 
    "Eye of newt and toe of frog,", 
    "Wool of bat and tongue of dog,",
    "Adder's fork and blindworm's sting,", 
    "Lizard's leg and howlet's wing.",
    "For charm of powerful trouble,",
    "Like a hell-broth boil and bubble.",
    "Double,double toil and trouble;",
    "Fire burn and couldron bubble.",
    "Scale of dragon,tooth of wolf,",
    "Witch's mummy, maw and gulf",
    "Of the ravin'd salt-sea shark,",
    "Root of hemlock digg'd in the dark,",
    "Liver of blaspheming Jew;",
    "Gall of goat; andslips of yew",
    "silver'd in the moon's eclipse;",
    "Nose of Turk, and Tartar's lips;",
    "Finger of birth-strangled babe",
    "Ditch-deliver'd by the drab,-",
    "Make the gruel thick and slab:",
    "Add thereto a tiger's chaudron,",
    "For ingrediants of our cauldron.",
    "Double,double toil and trouble,",
    "Fire burn and cauldron bubble.",
};

const int POEM_LINES = sizeof(POEMS) / sizeof(POEMS[0]);


bool getArgs(int argc, char* argv[], std::string* hostname, int* portno, bool* verbose) {
    try {
        std::cout << "Processing command line arguments" << std::endl;
        TCLAP::CmdLine cmd(argv[0], ' ', "1.0");
        TCLAP::ValueArg<std::string> hostnameArg("s", "hostname", "name of hosting the server", false, "localhost", "string");
        cmd.add(hostnameArg);
        TCLAP::SwitchArg verboseArg ("v", "verbose", "detail debugging output", false);
        cmd.add(verboseArg);
        TCLAP::ValueArg<int> portnoArg("p", "port", "well known port number", false, 7777, "int");
        cmd.add(portnoArg);

        cmd.parse(argc, argv);
        *hostname = hostnameArg.getValue();
        *verbose = verboseArg.getValue();
        *portno = portnoArg.getValue();

        std::cout << "hostname: " << *hostname << std::endl;
        std::cout << "portno: " << *portno << std::endl;
        std::cout << "verbose: " << (*verbose ? "True" : "False") << std::endl;
        return true;
    }
    catch (TCLAP::ArgException& e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return false;
    }
    return true;
}

class EchoServer {
public:
    EchoServer(std::string hostname, int portno);
    ~EchoServer();
    int serve();
protected:
    std::string hostname;
    int portno;
    int sock_listener;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t client_addr_size = sizeof(client_addr); 
    int sock_client;

    int getSocket();
    int bindSocket();
    int listenToCall();
    int acceptCall();
    void processRequest(int client_sock);
};

EchoServer::EchoServer(std::string hostname, int portno) {
    this->hostname = hostname;
    this->portno = portno;
}

int EchoServer::getSocket() {
    sock_listener = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_listener < 0) {
        std::cerr << "[ERROR] Socket cannot be created!\n";
        return -2;
    }
    return 0;
}

int EchoServer::bindSocket() {
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);
    inet_pton(AF_INET, "0.0.0.0", &server_addr.sin_addr);
  
    char buf[INET_ADDRSTRLEN];
    if (bind(sock_listener, (sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "[ERROR] Created socket cannot be binded to ( "
                  << inet_ntop(AF_INET, &server_addr.sin_addr, buf, INET_ADDRSTRLEN)
                  << ":" << ntohs(server_addr.sin_port) << ")\n";
        return -3;
    }

    std::cout << "[INFO] Sock is binded to ("  
              << inet_ntop(AF_INET, &server_addr.sin_addr, buf, INET_ADDRSTRLEN)
              << ":" << ntohs(server_addr.sin_port) << ")\n";
    return 0;
}

int EchoServer::listenToCall() {
    if (listen(sock_listener, SOMAXCONN) < 0) {
        std::cerr << "[ERROR] Socket cannot be switched to listen mode!\n";
        return -4;
    }
    std::cout << "[INFO] Socket is listening now.\n";
    return 0;
}

int EchoServer::acceptCall() {
    sock_client = accept(sock_listener, (sockaddr*)&client_addr, &client_addr_size);
    if (sock_client < 0) {
        std::cerr << "[ERROR] Connections cannot be accepted for a reason.\n";
        return -5;
    }
    std::cout << "[INFO] A connection is accepted now.\n";
    return 0;
}

void EchoServer::processRequest(int client_sock) {
    char msg_buf[4096];
    int bytes;
    while (true) {
        memset(msg_buf, 0, sizeof(msg_buf));
        bytes = recv(client_sock, msg_buf, 4096, 0);
        if (bytes <= 0) {
            std::cout << "[INFO] Client is disconnected.\n";
            break;
        }

        std::string message(msg_buf, bytes);
        std::string length_str = message.substr(0, message.find(' '));
        int length = std::stoi(length_str);
        std::string content = message.substr(message.find(' ') + 1);

        if (content == "EXIT" || content == "exit" || content == "Exit") {
            std::string response = "4 EXIT";
            send(client_sock, response.c_str(), response.size(), 0);
            close(client_sock);
            std::cout << "[INFO] Server is shutting down as per client request.\n";
            exit(0);
        } else {
            int line_index = length % POEM_LINES;
            std::string response = std::to_string(strlen(POEMS[line_index])) + " " + POEMS[line_index];
            send(client_sock, response.c_str(), response.size(), 0);
        }
    }

    close(client_sock);
    std::cout << "[INFO] Client socket is closed.\n";
}

int EchoServer::serve() {
    std::cout << "Start serving ..." << std::endl;
    if (getSocket()) return -2;
    if (bindSocket()) return -3;
    if (listenToCall()) return -4;

    while (true) {
        if (acceptCall()) return -5;

        if (fork() == 0) {
            close(sock_listener);
            processRequest(sock_client);
            exit(0);
        } else {
            close(sock_client);
        }
    }

    std::cout << "Done serving ..." << std::endl;
    return 0;
}

EchoServer::~EchoServer() {
    std::cout << "Echo Server is terminated.\n";
}

int main(int argc, char **argv) {
    bool verbose = false;
    std::string hostname;
    int portno;

    if (!getArgs(argc, argv, &hostname, &portno, &verbose)) {
        std::cout << "Errors in providing parameters/arguments. Program exits." << std::endl;
        exit(-1);
    }
    EchoServer echoServer(hostname, portno);
    return echoServer.serve();
}
