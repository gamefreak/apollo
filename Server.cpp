#include "eNetAdapt.h"
#include <map>
#include <apollo/Net/Net.h>
#include <apollo/Net/MessageDecode.h>
#include <apollo/Utilities/GameTime.h>

namespace Net
{

namespace Server
{

ENetHost* serverHost = NULL;
typedef std::map<unsigned int, ENetPeer*> ClientMap;
ClientMap clients;
static unsigned int nextClientID = 1;

const uint32_t SERVER_BANDWIDTH_LIMIT = 1024 * 64; // 64 kB/s
const int SERVER_MAX_CLIENTS = 8; // plucked this one out of my arse.

unsigned int badMessage[SERVER_MAX_CLIENTS+1][5]; //[clientID][n] = time bad packet was sent
unsigned int badMessageCount[SERVER_MAX_CLIENTS]; //number of bad packets sent

std::map<int, BlowfishKey> sessionKey;
std::map<int, BlowfishIV> sessionIV;
void makeBlowKey(int clientID)
{
//ADAM	sessionKey[clientID];
//ADAM	sessionIV[clientID];
	// add a random number generator for this
}
	
	
void Startup ( unsigned short port, const std::string& password )
{
	if (serverHost)
	{
		Shutdown();
	}
	ENetAddress addr;
	addr.port = htons(port);
	addr.host = INADDR_ANY;
	serverHost = enet_host_create(&addr, SERVER_MAX_CLIENTS, SERVER_BANDWIDTH_LIMIT, SERVER_BANDWIDTH_LIMIT);
}

void Shutdown ()
{
	if (serverHost)
	{
		clients.clear();
		enet_host_destroy(serverHost);
		serverHost = NULL;
	}
}

bool IsRunning ()
{
	return serverHost != NULL;
}

unsigned ClientCount ()
{
	return clients.size();
}

void KillClient ( unsigned int clientID ) //this is the nice disconnect
{
	ClientMap::iterator iter = clients.find(clientID);
	if (iter != clients.end())
	{
		enet_peer_disconnect_later(iter->second, 0);
		clients.erase(iter);
	}
	
}

void ChopClient (unsigned int clientID) //do not confuse with killClient, this is a harsh disconnect
	{
		ClientMap::iterator iter = clients.find(clientID);
		if (iter != clients.end())
		{
			enet_peer_reset(iter->second); //disconnect now, regardless of what the client does
			clients.erase(iter);
		}
	}

void SendMessage ( unsigned int clientID, const Message& msg )
{
	ClientMap::iterator iter = clients.find(clientID);
	if (iter != clients.end())
	{
		ENetPacket* packet = MessageEncoding::Encode(msg);
		enet_peer_send(iter->second, 0, packet);
	}
}

void BroadcastMessage ( const Message& msg )
{
	if (!serverHost)
		return;
	ENetPacket* packet = MessageEncoding::Encode(msg);
	enet_host_broadcast(serverHost, 0, packet);
}

bool IsConnected ( unsigned int clientID )
{
	return clients.find(clientID) != clients.end();
}

static void BadClient(unsigned int clientID) //deal with a bad message
{
	unsigned int currentTime = (unsigned int)GameTime();
    
    if(badMessageCount[clientID] > 1) 
    {
        if((currentTime - badMessage[clientID][badMessageCount[clientID]]) < 10)
        { // kick on 2nd in 10 seconds
            KillClient(clientID);
            return;
        }		
    }
    
    badMessageCount[clientID]++;
    badMessage[clientID][badMessageCount[clientID]] = currentTime;
}

	


Message* GetMessage ()
{
	ENetEvent event;
	if (enet_host_service(serverHost, &event, 0))
	{
		Message* msg = NULL;
		unsigned int clientID = 0;
		for (ClientMap::iterator iter = clients.begin(); iter != clients.end(); iter++)
		{
			if (iter->second == event.peer)
			{
				clientID = iter->first;
			}
		}
		switch (event.type)
		{
			case ENET_EVENT_TYPE_CONNECT:
				msg = new Message ( "CONNECT", NULL, 0 );
				clientID = nextClientID++;
				msg->clientID = clientID;
				clients[clientID] = event.peer;
				badMessage[clientID][0] = 0; //clear bad-message entry so it can be used
				Message* keymsg;
				Message* ivmsg;
				
				//make and share encryption keys
			//ADAM	makeBlowKey[clientID];
			//ADAM	keymsg = new Message( "BLOWKEY", sessionKey[clientID], 16 ); //make a message with the blowkey
			//ADAM	SendMessage( clientID, keymsg ); //send the blowkey
				
			//ADAM	ivmsg = new Message( "BLOWIV" , sessionIV[clientID], 8 ); //make a message with the blowIV
			//ADAM	SendMessage( clientID, ivmsg ); //send the IV
				break;
			case ENET_EVENT_TYPE_DISCONNECT:
				{
					msg = new Message ( "DISCONNECT", NULL, 0 );
					msg->clientID = clientID;
					ClientMap::iterator iter = clients.find(clientID);
					clients.erase(iter);
				}
				break;
			case ENET_EVENT_TYPE_RECEIVE:
				msg = MessageEncoding::Decode(event.packet);

				if(msg == NULL)
				{
					if (badMessageCount[clientID] >= 4)
					{
						KillClient(clientID);
					}
					else
					{
						BadClient(clientID);
					}
				}
				else 
                {
					msg->clientID = clientID;
				/*	
					if( Blowfish::do_decrypt(msg, sessionKey[clientID], sessionIV[clientID]) == false) // msg is passed by reference. Return 0 on error.
					{
				 //do something with the bad message
					} else {
						//do something with the recieved message
				 
					}
				 */ //disabled until finished implementation
				}
				
				enet_packet_destroy(event.packet);
				break;
		}
		return msg;
	}
	return NULL;
}

}

}
