###Missouri S&amp;T CS 5600 (Networking) Peer-to-Peer Project
=====

Description: 

A multi-threaded server facilitates peer-to-peer filesharing between multiple multi-threaded clients. Clients send a tracker files including its IP address, file name, chunk size and MD5 hash to server and those files are shared with other clients who are looking for that chunk. The clients then connect to each other with a 'threaded socket' to transfer the chunk. The clients intelligently pick which client should give the fastest download speed.

