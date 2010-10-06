#ifndef __apollo_net_h
#define __apollo_net_h

#include <string>
#include <string.h>

#include <eNetAdapt.h>

namespace Net
{
	typedef unsigned char BlowfishKey[16];
	typedef unsigned char BlowfishIV[8];
	
struct Message
{
	Message ( std::string _message, const void* _data, size_t _len ) : message(_message), dataLength(_len), clientID(0), data(NULL) { if (_data) { data = malloc(_len); memcpy(data, _data, _len); } }
	~Message () { if (data) free(data); }
	unsigned int clientID; // only for server use
	std::string message;
	void* data;
	size_t dataLength;
};

class Client
{
private:
	ENetHost* clientHost;
	ENetPeer* clientPeer;
	float badMessages[5];
	unsigned int badMessageCount;
	void BadMessage ();
public:
	Client ();
	~Client ();
	
	void Connect ( const std::string& host, unsigned short port, const std::string& password );
	void Disconnect ();
	bool IsConnected ();
	
	void SendMessage ( const Message& msg );
	Message* GetMessage ();
};

namespace Server
{

void Startup ( unsigned short port, const std::string& password );
void Shutdown ();
bool IsRunning ();
unsigned ClientCount ();

void KillClient ( unsigned int clientID );
bool IsConnected ( unsigned int clientID );

void SendMessage ( unsigned int clientID, const Message& msg );
void BroadcastMessage ( const Message& msg );
Message* GetMessage ();

}



namespace Blowfish {

	bool do_crypt( Message& msg, int ID);
	
	
}
}

#endif
