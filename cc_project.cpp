#include <algorithm>
#include <atomic>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

// Global variables
int listening_port;
vector<int> peer_ports;
int replies = 0;
priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;

class Peer {
public:
    Peer(int port, int num_peers, int current_process_index) : listening_port(port), num_peers(num_peers), current_process_index(current_process_index) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            cerr << "ERROR opening socket\n";
            exit(1);
        }

        bzero((char *)&serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port);

        if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("ERROR on binding \n");
            exit(1);
        }
    }

    bool canGoCritical() {
        return (replies == num_peers - 1 && pq.top().second == listening_port);
    }

    void executeCS() {
        cout << "Entered Critical Section!" << endl;
        usleep(8000000); // sleep for 8 seconds
        for (int port : peer_ports) {
            if (port == listening_port) continue;
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cerr << "Error opening socket.\n";
                return;
            }
            struct sockaddr_in serv_addr;
            bzero((char *)&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr.s_addr = INADDR_ANY;

            // Connect to the server
            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                cerr << "Error connecting to the server.\n";
                return;
            }

            string timestamp_str = "o\n" + to_string(0) + "\n" + to_string(listening_port) + "\n";

            send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
            // Close the socket
            cout << "Release message sent to " << port << endl;
            close(sockfd);
        }
    }

    void startAccept() {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
        if (newsockfd < 0) {
            cerr << "ERROR on accept\n";
            exit(1);
        }

        char buffer[1024];
        bzero(buffer, 1024);
        int n = recv(newsockfd, buffer, 1024, 0);
        if (n < 0) {
            cerr << "ERROR reading from socket\n";
            exit(1);
        }

        usleep(500000); // emulate delay

        int received_timestamp = -1;
        stringstream ss(buffer);
        string temp;
        int port;
        bool isReply = false, isReleased = false;
        while (getline(ss, temp, '\n')) {
            if (!temp.empty() && temp[0] == 'r') // reply
                isReply = true;
            else if (!temp.empty() && temp[0] == 'o') // open
                isReleased = true;
            else if (!temp.empty() && temp[0] == 'q') // request
                continue;
            else if (!temp.empty() && received_timestamp == -1)
                received_timestamp = stoi(temp);
            else if (!temp.empty())
                port = stoi(temp);
        }

        if (isReply) {
            if (port == listening_port) {
                replies += 1;
                cout << "Received a Reply!" << endl;
                if (canGoCritical()) {
                    replies = 0;
                    pq.pop();
                    thread cs(&Peer::executeCS, this);
                    cs.detach();
                }
            }
        } else if (isReleased) {
            pq.pop();
            cout << "Received a Release message!" << endl;
            if (canGoCritical()) {
                replies = 0;
                pq.pop();
                thread cs(&Peer::executeCS, this);
                cs.detach();
            }
        } else {
            int current_timestamp = timestamp;
            timestamp = max(current_timestamp, received_timestamp) + 1;

            cout << "Received a Request from " << port << endl;
            pq.push({received_timestamp, port});
            int sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                cerr << "Error opening socket.\n";
                return;
            }
            struct sockaddr_in serv_addr;
            bzero((char *)&serv_addr, sizeof(serv_addr));
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons(port);
            serv_addr.sin_addr.s_addr = INADDR_ANY;

            // Connect to the server
            if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                cerr << "Error connecting to the server.\n";
                return;
            }

            string timestamp_str = "r\n" + to_string(timestamp) + "\n" + to_string(port) + "\n";

            send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
            // Close the socket
            cout << "Reply sent to " << port << endl;
            close(sockfd);
        }
        close(newsockfd);
    }

    void startListening() {
        listen(sockfd, 5);
        while (true) {
            startAccept();
        }
    }

private:
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    atomic<int> timestamp{0};
    int current_process_index;
    int num_peers;
};

int main() {
    cout << "Enter the listening port for this process: ";
    cin >> listening_port;
    int num_peers;
    cout << "Enter the number of peers (excluding this): ";
    cin >> num_peers;

    if (num_peers < 1) {
        cerr << "Number of peers must be at least 1.\n";
        return 1;
    }

    for (int i = 0; i < num_peers; ++i) {
        cout << "Enter the port for peer " << i + 1 << ": ";
        int port;
        cin >> port;
        peer_ports.push_back(port);
    }

    // Add the listening port to the vector
    peer_ports.push_back(listening_port);

    // Sort the vector
    sort(peer_ports.begin(), peer_ports.end());

    // Find the index of the listening port
    int current_process_index = find(peer_ports.begin(), peer_ports.end(), listening_port) - peer_ports.begin();

    Peer p(listening_port, num_peers, current_process_index);
    p.startListening();

    return 0;
}
