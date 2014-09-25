CS5600_P2P
==========

Missouri S&amp;T CS 5600 (Networking) Peer-to-Peer Project

Protocol Description:
P2P peer program must handle 'createtracker' and 'updatetracker' commands in order for
the peer to be able to create / update tracker file at the tracker server. The protocol format 
of the messages between the peer program and the tracker server will be:

createtracker: message from peer to the tracker server

<createtracker filename filesize description md5 ip-address port-number>

createtracker: message from tracker server to the peer program

if command successful: <createtracker succ>

if command unsuccessful: <createtracker fail>

if tracker file already exists: <createtracker ferr>

Consider the same example described above, the protocol message for the message from 
the peer to the tracker server will look like:
<createtracker movie1.avi 109283519 Ghost_and_the_Darkness,_DVD_Rip
c68c2ee8bfca4e898b396e7a935a1d92 202.141.60.10 2097>

P2P peer program must also periodically update the tracker server with the fresh 
description of the files it is currently sharing. This is required because the file share might 
change at the peer as it is still in the process of downloading the file or it might decide to 
no longer share a particular file anymore. You must make the time interval between 
successive tracker refresh by the peer part of its configuration file. Make this parameter 15 minutes by default in the peer configuration file.

updatetracker: message from peer to the tracker server

<updatetracker filename start_bytes end_bytes ip-address port-number>

updatetracker: message from tracker server to the peer program

if tracker file does not exist: <updatetracker filename ferr>

if tracker file update successful: <updatetracker filename succ>

any other error / unable to update tracker file: <updatetracker filename fail>

You can make the peer program automatically send updatetracker messages to the 
tracker server for each file in the peer shared directory every 'n' seconds where 'n' is the 
refresh frequency time in the configuration file. P2P peer program must be able to contact the tracker server and request the list of tracker files maintained by it using LIST command. 

LIST – This command is sent by a connected peer to the tracker server to send over to the 
requesting peer the list of (tracker) files in the shared directory at the server. The format 
of the incoming message from the connected peer will be 
<REQ LIST>

In reply to the LIST request the server reply message structure must be:

<REP LIST X>
<1 file1name file1size file1MD5>\n
<2 file2name file2size file2MD5>\n
...
<x fileXname fileXsize fileXMD5>\n
<REP LIST END>

Additionally the P2P peer must be able to request the tracker server for a particular 
tracker file so that it can initiate the file transfer process. Protocol for the message sent by 
P2P peer to the tracker server in order to request a particular tracker:
<GET filename.track >

The server's response to the GET command must be:

<REP GET BEGIN>
<tracker_file_content >
<REP GET END FileMD5>

Once the P2P peer receives the reply for its tracker file request (GET), it must compare 
the MD5 checksum of the received data with the one contained as part of the protocol 
message. If MD5 matches, the tracker file data is assumed correct. The peer must save 
the tracker file into its local cache

The peer must be able to intelligently create peer requests using GET in order to get 
maximum download speed. Maximum data chunk size must be set to 1024 bytes. P2P 
peers' server thread handling the GET request by other peers must check for the chunk 
size requested and must enforce 1024 bytes upper limit strictly. In case the server thread 
at the peer receives a GET request for file chunk size greater than 1024 bytes, it must 
respond back with an error message <GET invalid>

Client/Peer must be able to create multiple threads, each thread requesting mutually 
exclusive file chunks from different peers, merging different chunks if possible after each 
thread terminates. This process of downloading the file must automatically start once the 
tracker file has been received successfully. The peer must be aware that the received 
tracker data might not be up to date as the tracker update period interval by peers may be 
large, so it should be prepared for failed TCP connection attempts made on some of the 
listed peers in the tracker file and deal with the situation intelligently. The peer must also 
be able to handle its own failure appropriately. That is peer program when executed must 
check into its local tracker cache and the shared file storage location to see if there
are still any incomplete files and must try to download the remaining bytes and not start 
the whole process over again. Once the file is successfully downloaded in its entirety, the 
peer must delete the corresponding tracker file stored in its local cache. The peer must 
contact the tracker server periodically [period of tracker update by peer should be
read from the configuration file]. The client must check for incomplete file chunks in its
local storage after each period elapses and should ask the tracker server for the latest 
tracker for only the files which are still incomplete.

Tracker Server: 
This is a multi-threaded centralized server whose primary job is to maintain a list of peers
sharing either partial or complete file chunks for each shared file in the P2P network.
Each new peer request connection should immediately be handed over to a worker thread 
that shall handle the request from that peer. Each worker thread serves only one peer at a 
time and terminates the connection as soon as the protocol reply message has been sent to 
the requesting peer. The commands received by the tracker server are: LIST, GET, 
createtracker, and updatetracker.

Upon receiving the commands, the tracker server behaves in the following ways:
1. LIST: sends the list of tracker files (with added information).
2. GET: sends the tracker file being requested.
3. createtracker: creates a tracker file with received information and time stamp, if the
same tracker file is not already created, and sends error message, otherwise.
4. updatetracker: if no such file exists it responds back by an error message to the peer,
closes the TCP connection and terminates the handler thread. If such tracker file exists,
then it creates a new entry if the peer is new (the time stamp for this new entry will the 
system's current time stamp) and updates the information if the peer is already added (its 
time stamp must be updated to the current system time stamp.). It also removes the entry 
of the dead peers. A peer is considered dead if its update time interval elapses. 
