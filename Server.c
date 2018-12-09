#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "Reliable.h"

int getPackets(char * file_path, struct packet * res);
struct input_server read_input(char * file_path);
void beginProcess(int num_packs, int sock, struct sockaddr_in clientAddr);
void sendACK(int next_seq, int sock, struct sockaddr_in addr);

struct packet packets[SEQNUM] = {0};
int servSock;
int clntSock;
int num_packets_glb;

int main(int argc, char *argv[]) {

    struct sockaddr_in servAddr;
    struct sockaddr_in clntAddr;

    unsigned short servPort;
    unsigned int clntLen;

    int recvMsgSize = 0;

    char buffer[BUFSIZ] = {0};


    struct input_server input = read_input("Server/server.in");

    /*Main Thread Setup*/
    servPort = input.portServer;   /* First arg: local port */

    /* Create socket for incoming connections */
    if ((servSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        DieWithError("socket () failed");

    /* Construct local address structure */
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(servPort);

    /* Construct local address structure */
    memset(&clntAddr, 0, sizeof(clntAddr));

    if (bind(servSock, (struct sockaddr *)&servAddr,sizeof(servAddr)) < 0)
        DieWithError("bind () failed");

    for (;;){
        char buffFileName[MAXNAME] = {0};
        int sizeCA = sizeof(clntAddr);

        //Wait until recieve the file name
        recvfrom(servSock, buffFileName, sizeof(buffFileName), 0, (struct sockaddr *) &clntAddr, &sizeCA);

        int thrd_num = fork();

        if (thrd_num == 0) {
            //Now in the Child Process
            close(servSock);

            if ((clntSock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
                DieWithError("Erroc Creating child Socket");

            int num_packs = getPackets(buffFileName, packets);

            if (num_packs > 0)
                beginProcess(num_packs, clntSock, clntAddr);
            else
                perror("Error reading the file");

            exit(0);
        } else if (thrd_num > 0) {
            //Now in the Parent Process
            close(clntSock);
            clntSock = -1;
        } else {
            //Error in Creating The Child
            perror("Error Creating the Child");
        }
    }

    exit(0);
}

int getPackets(char * file_path, struct packet * res) {

    FILE * fp;
    size_t bytesRead = 0;
    int num_packets = 0;
    unsigned char buffer[DATASIZE];  // array of bytes, not pointers-to-bytes

    fp = fopen(file_path, "r");
    if (fp == NULL) {
        return 0;
    }
    // read up to sizeof(buffer) bytes
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), fp)) > 0)
    {
        // process bytesRead worth of data in buffer
        struct packet packet;
        memset(&packet, 0, sizeof(packet));

        packet.len = bytesRead;
        packet.seqno = num_packets;
        packet.cksum = CHKSUM;
        strncpy(packet.data, buffer, sizeof(buffer));

        res[num_packets] = packet;
        num_packets++;
    }
    num_packets_glb = num_packets;
    fclose(fp);
    return num_packets;
}

struct input_server read_input(char * file_path) {
    FILE * fp;
    char * line;
    size_t len = 0;
    struct input_server inpt;
    memset(&inpt, 0, sizeof(inpt));

    fp = fopen(file_path, "r");
    if (fp == NULL)
        DieWithError("Input File Not Found");

    int counter = 0;
    while ((getline(&line, &len, fp)) != -1) {
        switch (counter) {
            case 0:
                inpt.portServer = atoi(line);
                break;
            case 1:
                inpt.max_window_size = atoi(line);
                break;
            case 2:
                inpt.seed = atoi(line);
                break;
            case 3:
                inpt.prob = atof(line);
                break;
        }
        counter++;
    }
    fclose(fp);
    return inpt;
}

void beginProcess(int num_packs, int sock, struct sockaddr_in clientAddr) {

    struct ack_packet ackPacket;
    //Send to the Client the ack with the file size
    sendACK(num_packs, sock, clientAddr);

    //TODO : Add timer for each packet
    //TODO: Start Timer

    //Wait for Ack
    int tr = sizeof(clientAddr);
    recvfrom(sock, &ackPacket, sizeof(struct ack_packet), 0, (struct sockaddr * ) &clientAddr, &tr);

    //TODO: Again handle any timer change

    //TODO: Real work start later
}

void sendACK(int next_seq, int sock, struct sockaddr_in addr) {
    struct ack_packet acknowldge;
    memset(&acknowldge, 0, sizeof(acknowldge));

    acknowldge.len = HEADERSIZE;
    acknowldge.ackno = next_seq;
    acknowldge.cksum = CHKSUM;

    sendto(sock, &acknowldge, sizeof(struct ack_packet), 0, (struct sockaddr*) &addr, sizeof(addr));

}