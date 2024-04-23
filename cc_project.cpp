#include <bits/stdc++.h>
#include <netinet/in.h>
#include <sys/socket.h>
using namespace std;

/*
typedef std::chrono::seconds TenSeconds;
const TenSeconds CC_time = 10s; // amount of time a device will spent in critical section
// change the above to increase or decrease the time spent in CS.
*/

typedef std::priority_queue<std::pair<int, int>, std::vector<std::pair<int, int>>, std::greater<std::pair<int, int>>> MinPriorityQueue;

int device_port;
vector<int> peer_ports;
int no_of_replies = 0;

// format of message exchanged between devices  [ type  timestamp port ]
// r for reply q for request  o for release

class Device
{
private:
  MinPriorityQueue pq;       // Priority queue to manage critical section requests
  int sockfd, newsockfd;
  socklen_t clilen;
  char buffer[256];
  struct sockaddr_in serv_addr, cli_addr;
  atomic<int> timestamp; 
  int current_process_index;
  vector<thread> threads;    // Vector to hold threads for listening, sending requests, and handling critical section

public:
  Device(int port, int adj_devices, int cprocess_index)
  {

    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0)
    {
      cerr << "ERROR in opening socket\n";
      exit(1);
    }
    current_process_index = cprocess_index;
    for (int i = 0; i < adj_devices; ++i)
    {
      timestamp = 0;
    }
  
    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
      perror("ERROR : Binding Failed \n");
      exit(1);
    }
  }

// Start the device
  void start()
  {
    beginListening();       // Start listening for incoming connections
    triggerEvent();         // Start triggering events (e.g., sending requests)
    syncThreads();          // Synchronize threads
  }

  
  void static ExecuteCS()
  {
    cout << "Entered into CRITICAL SECTION!" << endl;
    usleep(40000000); 
    for (int port : peer_ports)
    {
      if (port == device_port)
        continue;
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in serv_addr;
      bzero((char *)&serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&serv_addr,
                  sizeof(serv_addr)) < 0)
      {
        cerr << "Error in connecting to the server.\n";
        return;
      }

      string timestamp_str = "o\n" + to_string(0) + "\n" + to_string(device_port) + "\n";

      send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
      // Close the socket 
      cout << "Release message sent to " << port << endl;
      close(sockfd);
    }
  }

  void startAccept(int current_process_index)
  {

    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    if (newsockfd < 0)
    {
      cerr << "ERROR on accept\n";
      exit(1);
    }

    char buffer[1024];
    bzero(buffer, 1024);
    int a = recv(newsockfd, buffer, 1024, 0);
    if (a < 0)
    {
      cerr << "ERROR reading from socket\n";
      exit(1);
    }
    
    usleep(600000);

    int received_timestamp = -1;
    stringstream ss(buffer);
    string temp;
    int port;
    bool isReply = false, isReleased = false;
    while (getline(ss, temp, '\n'))
    {
       if(!temp.empty() && temp[0] == 'r')  
       { 
        isReply = true;
       }
      else if (!temp.empty() && temp[0] == 'o') 
        {
            isReleased = true;
        }
      else if (!temp.empty() && temp[0] == 'q') 
       { 
        continue;
       }
      else if (!temp.empty() && received_timestamp == -1)
       { 
        received_timestamp = (stoi(temp));
       }
      else if (!temp.empty())
        {
            port = stoi(temp);
        }
    }

    // Compare the received timestamps with the current timestamps
    if (isReply)
    {
      if (port == device_port)
      {
        no_of_replies += 1;
        cout << "Got a Reply!" << endl;
        if (EnterCritical_section())
        {
          no_of_replies=0;
          pq.pop();
          thread cs(ExecuteCS);
          cs.detach();
        }
      }
    }
    else if (isReleased)
    {
      pq.pop();
      cout << "Got a Release message!" << endl;
      if (EnterCritical_section())
      {
        no_of_replies=0;
        pq.pop();
        thread cs(ExecuteCS);
        cs.detach();
      }
    }
    else
    {
      int current_timestamp = timestamp;
      timestamp = max(current_timestamp, received_timestamp) + 1;
      
      cout << "Got a Request from " << port << endl;
      pq.push(pair<int, int>(received_timestamp, port));
      int sockfd = socket(AF_INET, SOCK_STREAM, 0);
      if (sockfd < 0)
      {
        cerr << "Error opening socket.\n";
        return;
      }
      struct sockaddr_in serv_addr;
      bzero((char *)&serv_addr, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      // Connect to the server
      if (connect(sockfd, (struct sockaddr *)&serv_addr,
                  sizeof(serv_addr)) < 0)
      {
        cerr << "Error connecting to the server.\n";
        return;
      }
     
      string timestamp_str = "r\n" + to_string(timestamp) + "\n" + to_string(port) + "\n";

      send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
      // Close the socket
      cout << " Reply sent to " << port << endl;
      close(sockfd);
    }
    close(newsockfd);
  }
  void beginListening()
  {

    int index = current_process_index;
    listen(sockfd, 5);
    threads.push_back(thread([this, index]()
                             {
      
      while (true) {
        startAccept(index);
      } }));
  }

  void triggerEvent()
  {
    int index = current_process_index;
    threads.push_back(thread([this, index]()
                             {
      while (true) {
        int choice;
        cout << "Enter 1 to request Critical Section, or \n2 to print the critical section requests or \n3 to exit .\n";

        cin >> choice;
        if (choice == 1) {
          timestamp++;
          pq.push(pair<int,int>(timestamp,device_port));
          
          for(int port:peer_ports)
          {
            if(port==device_port)
              continue;
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
            if (connect(sockfd, (struct sockaddr *)&serv_addr,
                        sizeof(serv_addr)) < 0) {
              cerr << "Error connecting to the server.\n";
            return;
            }

            string timestamp_str="q\n"+to_string(timestamp)+"\n"+to_string(device_port)+"\n";
            
            send(sockfd, timestamp_str.c_str(), timestamp_str.size(), 0);
            // Close the socket
            cout  << "Request sent to "<<port<<endl;
            close(sockfd);
            
          }
          
          
        } else if (choice == 2) {
          vector<pair<int,int>>temp;
          while(!pq.empty())
          {
            temp.push_back(pq.top());
            cout<<pq.top().first<<":"<<pq.top().second<<endl;
            pq.pop();
          }
          for(int i=0;i<temp.size();i++)
            pq.push(temp[i]);
        } else if (choice == 3) {
          
          exit(0);
        }
      } }));
  }


  void syncThreads()
  {
    for (auto &thread : threads)
    {
      if (thread.joinable())
      {
        thread.join();
      }
    }
  }
 
     bool EnterCritical_section()
  {
    if (no_of_replies == peer_ports.size() - 1 && pq.top().second == device_port)
      return true;
    return false;
  }

};

int main()
{
 int no_of_devices ;
cout << "Enter the no of devices : ";
cin>>no_of_devices;
if (no_of_devices< 2)
  {
    cerr << "Number of devices must be atleast 2 .\n";
    return 1;
  }
  cout << "Please enter the port number on which this device will listen for incoming connections : ";

  cin >> device_port;

  peer_ports.push_back(device_port);
  
  for (int i = 1; i < no_of_devices; ++i)
  {
    cout << "Enter the port for peer device : " << i  << ": ";
    int t;
    cin >> t;
    peer_ports.push_back(t);
  }
  sort(peer_ports.begin(), peer_ports.end());
  int process_index = find(peer_ports.begin(), peer_ports.end(), device_port) -peer_ports.begin();


  Device d(device_port, no_of_devices-1, process_index);
  d.start();
 
  return 0;

}
