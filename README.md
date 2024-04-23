
**Algorithm**

**Requesting process**
1.	Pushing its request in its own queue (ordered by time stamps)
2.	Sending a request to every node.
3.	Waiting for replies from all other nodes.
4.	If own request is at the head of its queue and all replies have been received, enter critical section.
5.	Upon exiting the critical section, remove its request from the queue and send a release message to every process.

**Other processes**

1.After receiving a request, pushing the request in its own request queue (ordered by time stamps) and reply with a time stamp.
2.After receiving release message, remove the corresponding request from its own request queue.

