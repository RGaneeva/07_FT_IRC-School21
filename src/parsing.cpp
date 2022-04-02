#include "server.hpp"
#include <vector>
#include <algorithm>
#include <iostream>
using namespace std;
#include <stdio.h>
#include <cstring>
int Server::Find(string &str)
{
    int ravno = 0;
    for (int i = 0;i<users.size();i++)
    {
        if (strlen(users[i].c_str()) >= strlen(str.c_str()))
        {
            for (int j =0;j<strlen(str.c_str());j++)
            {
                if (users[i][j] == str[j])
                    ravno++;
            }

            if (ravno == strlen(str.c_str()) && strlen(users[i].c_str()) == strlen(str.c_str()))
                return 1;
            ravno = 0;
        }
    }
    return 0;
}

int Server::Find(string &str, string str2)
{
    int res = -1;int ravno = 0;
    int len1 = strlen(str.c_str());
    int len2 = strlen(str2.c_str());
    if (len1 >= len2)
    {
        for (int i = 0;i<len2;i++)
        {
            if (str[i] == str2[i])
                ravno++;
            else
                ravno = 0;
        }
        if (ravno == len2)
        {
            res = 0;
            return res;
        }
    }
    return res;
}

int Server::checkClient(string str)
{
    int k = Find(str);
    if (k == 1)
    {
        printf("<%s> :Nickname is already in use\n",str.c_str());//436???
        return 1;
    }
    else
        return 0;
}

int Server::cmdNICK(string str, int n, struct kevent &event)//доб. замену ника
{
    string nick(str.substr(str.find_first_of(' ') + 1));
    size_t found = nick.find('\n');
    if (found > 0)
        nick.erase(found);

    // vector<string> buff;
    // int j = 0;
    // for (int i = n; i < str.length();i++)
    // {
    //     if (str[i] == ' ')
    //     {
    //         i++;
    //         int k = i;
    //         for (int h = 0;h< str.length() - i;h++)
    //         {
    //             if(str[k] >= 33 && str[k] <= 126)
    //             {
    //                 nick[h] = str[k];
    //                 k++;
    //             }
    //         }
    //         break;
    //     }
    // }

    printf("%s", nick.c_str());
    if (nick.length() < 1)
        ERR(":No nickname given", strerror(errno));//431???
    if (checkClient(nick) == 0)
    {
        printf("check: %s\n", nick.c_str());
        users.push_back(nick);
        fds.push_back(event);
        return 0;
    }
    else
        return 1;
}

int Server::parsBuffer(string str, struct kevent &event)
{
    int in = str.find("NICK");
    printf("%d\n", in);
    int ret = 0;
    if (Find(str, "NICK") == 0)
    {
        // cout << "hello\n";
        ret = cmdNICK(str, in, event);
    }
    if (Find(str, "PING") == 0)
        sendAnswer(event,"PONG 10.21.21.52");
    if (Find(str, "QUIT") == 0)
    {
        printf("users: %lu\n", users.size());
        printf("fds: %lu\n", fds.size());
        vector<string>::iterator it2 = users.begin();
        for (vector<struct kevent>::iterator it = fds.begin();it!=fds.end() || it2!=users.end();)
        // for (vector<struct kevent>::iterator it = fds.begin();it!=fds.end() || it2!=users.end();it++, it2++)
        {
            for(int i=0;i<users.size();i++)
                printf("vector: %s\n", users[i].c_str());
        	if (it->ident == event.ident)
        	{
                printf("users: %lu\n", users.size());
                printf("fds: %lu\n", fds.size());
                printf("in_users: %s\n", it2->c_str());
                printf("in_fds: %lu\n", it->ident);
                if (users.size() == 1)
                {
                    printf("users_size: %lu\n", users.size());
                    users.clear();
                    fds.clear();
                    break;
                }
                else
                {
                    printf("users_size_erase: %lu\n", users.size());
                    users.erase(it2);
                }
                if (fds.size() == 1)
                {
                    std::cout << "asdasd";
                    users.clear();
                    fds.clear();
                    break;
                }
                else
                {
                    printf("fds_size_erase: %lu\n", fds.size());
        		    fds.erase(it);
                }
        	}
            it2 = users.begin();
            it = fds.begin();
        	printf("----------------\n");
        }
        onClientDisconnect(event);
    }
    if (Find(str,"LIST") == 0)
    {
        printf("users: %lu\n", users.size());
        printf("fds: %lu\n", fds.size());
        for (int i = 0; i<users.size();i++)
            printf("%s\n", users[i].c_str());
        for (int i = 0;i<fds.size();i++)
            printf("%lu\n", fds[i].ident);
    }
    return ret;
}