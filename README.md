Algorithm
Nodal properties
Every process maintains a queue of pending requests for entering critical section in order. The queues are ordered by virtual time stamps derived from Lamport timestamps.[1]
Algorithm
Requesting process

Pushing its request in its own queue (ordered by time stamps)
Sending a request to every node.
Waiting for replies from all other nodes.
If own request is at the head of its queue and all replies have been received, enter critical section.
Upon exiting the critical section, remove its request from the queue and send a release message to every process.
Other processes

After receiving a request, pushing the request in its own request queue (ordered by time stamps) and reply with a time stamp.
After receiving release message, remove the corresponding request from its own request queue.
