#include "Net.h"
#include "eNetAdapt.h"

namespace Net
{

namespace MessageEncoding
{

ENetPacket* Encode ( const Message& message );
Message* Decode ( ENetPacket* packet );

}

}
