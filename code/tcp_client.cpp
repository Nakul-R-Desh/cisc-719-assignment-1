#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <tclap/CmdLine.h>

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
}

class EchoClient {
public:
    EchoClient(std::string, int);
    ~EchoClient();
    int connectServer();
    int processRequest();

protected:
    std::string hostname;
    int portno;
    sockaddr_in server_addr;
    hostent *server;
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
};

EchoClient::EchoClient(std::string hostname, int portno) {
    this->hostname = hostname;
    this->portno = portno;
}

EchoClient::~EchoClient() {
    std::cout << "Client is terminated ...\n";
}

int EchoClient::connectServer() {
    if (sock_fd < 0) {
        std::cerr << "[ERROR] Socket cannot be created!\n";
        return -2;
    }

    std::cout << "[INFO] Socket has been created.\n";

    server = gethostbyname(hostname.c_str());
    if (!server) {
        std::cerr << "[ERROR] No such host!\n";
        return -3;
    }
    std::cout << "[INFO] Hostname is found.\n";

    std::memset((char*)&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portno);

    if (server->h_addr_list[0])
        std::memcpy((char*)server->h_addr_list[0], (char*)&server_addr.sin_addr, server->h_length);
    else {
        std::cerr << "[ERROR] There is no a valid address for that hostname!\n";
        return -5;
    }

    if (connect(sock_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Connection cannot be established!\n";
        return -6;
    }
    std::cout << "[INFO] Connection established.\n";
    return 0;
}

int EchoClient::processRequest() {
    char buf[4096];
    std::string temp;
    do {
        std::memset(buf, 0, 4096);
        std::cout << "> ";
        std::getline(std::cin, temp, '\n');

        std::string formatted_message = std::to_string(temp.size()) + " " + temp;
        std::strcpy(buf, formatted_message.c_str());

        if (!strlen(buf)) continue;
        else if (!strcmp(buf, "quit")) break;

        int bytes_send = send(sock_fd, buf, strlen(buf), 0);
        if (bytes_send < 0) {
            std::cerr << "[ERROR] Message cannot be sent!\n";
            break;
        }

        int bytes_recv = recv(sock_fd, buf, 4096, 0);
        if (bytes_recv < 0) {
            std::cerr << "[ERROR] Message cannot be received!\n";
        } else if (bytes_recv == 0) {
            std::cout << "[INFO] Server is disconnected.\n";
        } else {
            std::cout << "server> " << std::string(buf, 0, bytes_recv) << "\n";
        }
        if (!strcmp(buf, "END")) {
            break;
        }

    } while (true);

    close(sock_fd);
    std::cout << "[INFO] Socket is closed.\n";
    return 0;
}

int main(int argc, char **argv) {
    bool verbose = false;
    std::string hostname;
    int portno;

    if (!getArgs(argc, argv, &hostname, &portno, &verbose)) {
        std::cout << "Errors in providing parameters/arguments. Program exits." << std::endl;
        exit(-1);
    }
    EchoClient echoClient(hostname, portno);

    int retval = echoClient.connectServer();
    if (retval) {
        return retval;
    }
    retval = echoClient.processRequest();
    if (retval) {
        return retval;
    }
    return 0;
}