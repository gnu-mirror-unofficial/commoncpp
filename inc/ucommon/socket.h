#ifndef _UCOMMON_SOCKET_H_
#define	_UCOMMON_SOCKET_H_

#ifndef _UCOMMON_TIMERS_H_
#include <ucommon/timers.h>
#endif

#ifndef	_UCOMMON_LINKED_H_
#include <ucommon/linked.h>
#endif

#ifdef	_MSWINDOWS_
#include <ws2tcpip.h>
#include <winsock2.h>
#define	SHUT_RDWR	SD_BOTH
#define	SHUT_WR		SD_SEND
#define	SHUT_RD		SD_RECV
#else
#include <unistd.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netdb.h>
typedef	int SOCKET;
#define	INVALID_SOCKET -1
#endif

#ifndef IPTOS_LOWDELAY
#define IPTOS_LOWDELAY      0x10
#define IPTOS_THROUGHPUT    0x08
#define IPTOS_RELIABILITY   0x04
#define IPTOS_MINCOST       0x02
#endif

typedef	struct
{
	union
	{
		struct in_addr ipv4;
		struct in6_addr ipv6;
	};
}	inethostaddr_t;

#ifdef	AF_INET6
struct sockaddr_internet
{
	unsigned short sa_family;
	char sa_data[sizeof(struct sockaddr_in6) - 2];
};
#else
struct sockaddr_internet
{
	unsigned short sa_family;
	char sa_data[sizeof(struct sockaddr_in) - 2];
};

struct sockaddr_storage
{
	unsigned short sa_family;
#ifdef	AF_UNIX
	char sa_data[104];
#else
	char sa_data[sizeof(struct sockaddr_in) - 2];
#endif
};
#endif

NAMESPACE_UCOMMON

class __EXPORT cidr : public LinkedObject
{
protected:
	int family;
	inethostaddr_t netmask, network;

	unsigned getMask(const char *cp) const;

public:
	cidr();
	cidr(const char *str);
	cidr(LinkedObject **policy, const char *str);
	cidr(const cidr &copy);

	static bool allow(SOCKET so, cidr *accept, cidr *reject);
	static bool find(cidr *policy, const struct sockaddr *addr);

	inline int getFamily(void) const
		{return family;};

	inline inethostaddr_t getNetwork(void) const
		{return network;};

	inline inethostaddr_t getNetmask(void) const
		{return netmask;};

	inethostaddr_t getBroadcast(void) const;

	unsigned getMask(void) const;
		
	void set(const char *c);

	bool isMember(const struct sockaddr *saddr) const;

	inline bool operator==(const struct sockaddr *saddr) const
		{return isMember(saddr);};
		
	inline bool operator!=(const struct sockaddr *saddr) const
		{return !isMember(saddr);}; 
};

class __EXPORT Socket
{
protected:
	Socket();

	SOCKET so;

	unsigned getPending(void) const;

public:
	Socket(const Socket &s);
	Socket(SOCKET so);
	Socket(int family, int type, int protocol = 0);
	Socket(const char *iface, const char *port, int family, int type, int protocol = 0);
	virtual ~Socket();

	void release(void);
	bool isPending(unsigned qio) const;
	bool isConnected(void) const;
	bool waitPending(timeout_t timeout = 0) const;
	bool waitSending(timeout_t timeout = 0) const;

	int setBroadcast(bool enable);
	int setKeepAlive(bool enable);
	int setNonBlocking(bool enable);
	int setMulticast(bool enable);
	int getError(void);
	int setTimeToLive(unsigned char ttl);
	int setTypeOfService(unsigned tos);

	inline void shutdown(void)
		{::shutdown(so, SHUT_RDWR);};

	int connect(const char *host, const char *svc);
	bool create(int family, int type, int protocol = 0);

	int disconnect(void);
	int join(const char *member);
	int drop(const char *member);

	size_t peek(void *data, size_t len) const;

	virtual ssize_t get(void *data, size_t len, struct sockaddr *from = NULL);

	virtual ssize_t put(const void *data, size_t len, struct sockaddr *to = NULL);

	virtual ssize_t gets(char *data, size_t max, timeout_t to = Timer::inf);

	ssize_t puts(const char *str);

	operator bool();

	bool operator!() const;

	Socket &operator=(SOCKET s);

	inline operator SOCKET() const
		{return so;};

	inline SOCKET operator*() const
		{return so;};
};

class __EXPORT ListenSocket : protected Socket
{
public:
	ListenSocket(const char *iface, const char *svc, unsigned backlog = 5);

	SOCKET accept(struct sockaddr *addr = NULL);

	inline bool waitConnection(timeout_t timeout = Timer::inf) const
		{return Socket::waitPending(timeout);};

    inline operator SOCKET() const
        {return so;};

    inline SOCKET operator*() const
        {return so;};
};

END_NAMESPACE

extern "C" {
	__EXPORT void cpr_closesocket(SOCKET so);
	__EXPORT int cpr_joinaddr(SOCKET so, const char *host);
	__EXPORT int cpr_dropaddr(SOCKET so, const char *host);
	__EXPORT int cpr_bindaddr(SOCKET so, const char *host, const char *svc);
	__EXPORT int cpr_disconnect(SOCKET so);
	__EXPORT int cpr_connect(SOCKET so, const char *host, const char *svc);
	__EXPORT char *cpr_hosttostr(struct sockaddr *sa, char *buf, size_t max);
	__EXPORT struct addrinfo *cpr_getsockhint(SOCKET so, struct addrinfo *h);
	__EXPORT socklen_t cpr_getsockaddr(SOCKET so, struct sockaddr_storage *addr, const char *host, const char *svc);
	__EXPORT socklen_t cpr_getaddrlen(struct sockaddr *addr);
};

#endif
