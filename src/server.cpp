#include "server.hpp"
#include <iostream>
using namespace std;
// Основной конструктор класса, в качестве параметров принимает ip, port, и максимальную длину очереди ожидающих соединений.
Server::Server(const char *addr, int port, int backlog) :
m_address(),m_sock(),m_backlog(backlog),
m_sock_reuse(1),m_kqueue(),m_event_subs(),
m_event_list(),m_receive_buf(),m_sock_state()
{
    // Заполняю структуру значениями ip, port и создаю сокет.
	m_address.sin_family = AF_INET;
	m_address.sin_addr.s_addr = inet_addr(addr);
	m_address.sin_port = htons(port);
	m_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sock < 0)
	{
		ERR("socket: %s", strerror(errno));
		return;
	}
    // Это нужно чтобы сразу можно было пересоздать сервер на нужном адресе.
	m_sock_state = INITIALIZED;
	m_sock_reuse = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, &m_sock_reuse,
		sizeof(m_sock_reuse));
    // Делаю фдешник не блокирующимся.
	fcntl(m_sock, F_SETFL, O_NONBLOCK);
}

// Server & Server::operator=(Server const & copy) 
// {
// 	// m_sock_reuse = copy.m_sock_reuse;
//     // m_sock = copy.m_sock;
//     // m_backlog = copy.m_backlog;
//     // m_kqueue = copy.m_kqueue;
// 	users = copy.users;
// 	for (int i = 0; i < users.size(); i++)
// 		users[i]=copy.users[i];
//     fds = copy.fds;
// 	for (int i = 0; i < fds.size(); i++)
// 		fds[i]=copy.fds[i];
// 	return (*this);
// }

int Server::bind()
{
    // bind привязывает к сокету адрес из структуры m_address с его длинной.
	int err = ::bind(m_sock, (struct sockaddr *) &m_address,
		sizeof(m_address));
	if (err < 0)
		ERR("bind: %s", strerror(errno));
	else
		m_sock_state = BOUND;
	return err;
}

int Server::listen()
{
    // Тут слушаем сокет
	int err = ::listen(m_sock, m_backlog);
	if (err < 0)
		ERR("listen: %s", strerror(errno));
	else
		m_sock_state = LISTENING;
	return err;
}

int Server::initServer()
{
	int err = 0;
	if (m_sock_state < BOUND)
	{
		if ((err = bind()) < 0)
			return err;
		if ((err = listen()) < 0)
			return err;
	}
    // kqueue предоставляет механизм уведомления процесса о некоторых событиях, произошедших в системе.
    // kqueue получает дескриптор очереди сообщений ядра (в случае ошибки процесс завершается).
	m_kqueue = kqueue();
    // EV_SET это просто макрос, который заполянет элементы структуры kevent.
    // m_sock - дескриптор файла, за изменениями которого мы хотим наблюдать.
    // EV_ADD - добавить событие, EVFILT_READ - тип фильтра - в данном случае мы следим за чтением.
	EV_SET(&m_event_subs, m_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
    // регистрация события
	err = kevent(m_kqueue, &m_event_subs, 1, NULL, 0, NULL);
	if (err < 0)
	{
		ERR("kqueue setup: %s", strerror(errno));
		ERR("  m_sock: %d", m_sock);
		ERR("  m_kqueue: %d", m_kqueue);
	}
	return err;
}

void Server::startServer()
{
    // функция запуска сервера
	int event_count = 0;
	int event_iter = 0;
	struct kevent curr_event;
	if (initServer() < 0 && m_sock_state == LISTENING)
	{
		ERR("aborting run loop");
		return;
	}
	while(1)
	{ 
		event_count = kevent(m_kqueue, NULL, 0, m_event_list, 32, NULL);
		if (event_count < 1)
		{
			ERR("kevent read: %s", strerror(errno));
			return;
		}
		for (event_iter = 0; event_iter < event_count; event_iter++)
		{
			curr_event = m_event_list[event_iter];
			if (curr_event.ident == m_sock)
				onClientConnect(curr_event);
			else
			{
				if (curr_event.flags & EVFILT_READ) 
					onRead(curr_event);
				if (curr_event.flags & EV_EOF)
					onEOF(curr_event);
			}
		}
	}
}

int Server::onClientConnect(struct kevent& event)
{
    /*Функция accept используется с сокетами, 
    ориентированными на устанавление соединения (SOCK_STREAM, SOCK_SEQPACKET и SOCK_RDM).
     Эта функция извлекает первый запрос на соединение из очереди ожидающих соединений,
    создаёт новый подключенный сокет почти с такими же параметрами, что и у s, 
    и выделяет для сокета новый файловый дескриптор, который и возвращается. 
    Новый сокет более не находится в слушающем состоянии. Исходный сокет s не изменяется при этом вызове.
    Заметим, что флаги файловых дескрипторов (те, что можно установить с помощью параметра F_SETFL функции fcntl, 
    типа неблокированного состояния или асинхронного ввода-вывода) не наследуются новым файловым дескриптором после accept.*/
	int client_sock = ::accept(event.ident, NULL, NULL);
	DEBUG("[0x%016" PRIXPTR "] client connect", (unsigned long) client_sock);
	if (client_sock < 0)
	{
		ERR("[0x%016" PRIXPTR "] client connect: %s", event.ident, 
			strerror(errno));
	}
	fcntl(client_sock, F_SETFL, O_NONBLOCK);
	EV_SET(&m_event_subs, client_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
	int err = kevent(m_kqueue, &m_event_subs, 1, NULL, 0, NULL);
	if (err < 0)
	{
		ERR("[0x%016" PRIXPTR  "] sub: %s", event.ident, strerror(errno));
	}
	return err;
}

int Server::onClientDisconnect(struct kevent& event)
{
    // Функция очищает структуру событий и закрывает конект.
	DEBUG("[0x%016" PRIXPTR "] client disconnect", event.ident);
	EV_SET(&m_event_subs, event.ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
	int err = kevent(m_kqueue, &m_event_subs, 1, NULL, 0, NULL);
	if (err < 0)
		ERR("[0x%016" PRIXPTR "] kqueue unsub", event.ident);
	return ::close(event.ident);
}

void Server::onRead(struct kevent& event)
{
	DEBUG("[0x%016" PRIXPTR "] client read", event.ident);
    // Функция recv служит для чтения данных из сокета.
    // Первый аргумент - сокет-дескриптор,
    // Второй и третий аргументы - адрес и длина буфера для записи читаемых данных, 
    // Четвертый параметр - это комбинация битовых флагов, управляющих режимами чтения.
	int bytes_read = recv(event.ident, m_receive_buf, 
		sizeof(m_receive_buf) - 1, 0);
	if (bytes_read <= 0)
	{
		ERR("[0x%016" PRIXPTR "] client receive: %s", event.ident, 
			strerror(errno));
		return;
	}
    // Сюда надо будет вставить обработчик сокетов.
    // Печатаю информацию из сокета полученного от клиента.
	m_receive_buf[bytes_read] = '\0';
	int pars = parsBuffer(m_receive_buf, event);
	cout << "-------------------------------\n";
	for (int i = 0;i< bytes_read;i++)
		cout << m_receive_buf[i];
	cout << "-------------------------------"<< endl;
	DEBUG("%s", m_receive_buf);
    // Отправляю ответ.
	if (pars == 0)
	{
		sendAnswer(event, SUCCESSCONNECT);
		cout << "-------------------------------\n";
		cout << "users online: " << users.size() << endl;
		for (int i = 0;i<users.size();i++)
			printf("%s\n",users[i].c_str());
		cout << "-------------------------------\n";
	}
	else
	{
		sendAnswer(event, ERROR);
		// sendAnswer(event, ": 451 :You have not registered\t\n");
		onClientDisconnect(event);
	}
	event.flags |= EV_EOF;
}

void Server::onEOF(struct kevent& event)
{
	DEBUG("[0x%016" PRIXPTR "] client eof", event.ident);
}
	
int Server::close()
{
    // Функция для закрытия фдешника.
	int err = ::close(m_sock);
	if (err < 0)
		ERR("close: %s", strerror(errno));
	return err;
}

Server::~Server() {if (m_sock_state == LISTENING) close();}

void Server::sendAnswer(struct kevent &event, string str)
{
	int bytes_sent = send(event.ident, str.c_str(), str.size(), 0);
}