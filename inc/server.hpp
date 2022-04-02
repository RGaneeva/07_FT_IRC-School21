#ifndef SERVER_HPP
#define SERVER_HPP
#include <string>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>
#include <errno.h>
#include <inttypes.h>
#include <fcntl.h>
#include <fstream>
#include "log.hpp"
#include <iostream>
#include <vector>
using namespace std;
#define ERROR ": 433 Nickname is already in use\r\n"
#define SUCCESSCONNECT ":server 376 ->\r\n"

class Server
{
  public:
    Server(const char *addr, const int port, int backlog);
    Server & operator=(const Server & copy);
    ~Server();
    void onRead(struct kevent& event);
    void onEOF(struct kevent& event);
    int onClientConnect(struct kevent& event);
    int onClientDisconnect(struct kevent& event);
    void startServer();
    int parsBuffer(string str, struct kevent &event);
    int cmdNICK(string str, int n, struct kevent &event);
    int checkClient(string str);
    int Find(string &str);
    int Find(string &str, string str2);
    void sendAnswer(struct kevent &event, string str);
  private:
    int listen();
    int bind();
    int shutdown();
    int close();
    int initServer();
    struct sockaddr_in m_address;
    int m_sock_reuse;
    int m_sock;
    int m_backlog;
    int m_kqueue;
    struct kevent m_event_subs;
    struct kevent m_event_list[32];
    char m_receive_buf[1024];
    vector<string> users;
    vector<struct kevent> fds;
    enum SocketState 
    {
      INITIALIZED,
      BOUND,
      LISTENING,
      CLOSED
    };
    SocketState m_sock_state;
};
#endif