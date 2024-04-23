Y.Santosh Reddy 20CS01027
M.Tharun Thej 20CS01037
C.Muni Siva charan 20CS01015

**Lamoprts mutual exclusion Algorithm**

**Requesting process**
1.	Pushing its request in its own queue (ordered by time stamps)
2.	Sending a request to every node.
3.	Waiting for replies from all other nodes.
4.	If own request is at the head of its queue and all replies have been received, enter critical section.
5.	Upon exiting the critical section, remove its request from the queue and send a release message to every process.

**Other processes**

1.After receiving a request, pushing the request in its own request queue (ordered by time stamps) and reply with a time stamp.
2.After receiving release message, remove the corresponding request from its own request queue.

**_Implemented Lamport's distributed mutual exclusion algorithm in C++. This program creates a network of devices where each device can request access to a critical section. Devices communicate through sockets and exchange messages to get access to the critical section. The algorithm ensures mutual exclusion, progress, and fairness among devices competing for the critical section._**


**_Compile the C++ code using the following command_**

 **g++  cc_project.cpp**
 
 **_Run the compiled program in 3 terminals on Linux: using_**

 **./a.out**

 **_Enter the port to listen on and the number of other peers in the network, along with their port numbers_**
 
 **_Please review the output file for more details_**


 
  



