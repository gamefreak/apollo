PROJECT(ENET)

SET(ENET_HEADERS
	./enet/callbacks.h
	./enet/enet.h
	./enet/list.h
	./enet/nettime.h
	./enet/protocol.h
	./enet/types.h
	./enet/utility.h
	)

IF (UNIX)
	SET(ENET_HEADERS ${ENET_HEADERS} ./enet/unix.h)
ELSEIF(WIN32)
	SET(ENET_HEADERS ${ENET_HEADERS} ./enet/win32.h)
ENDIF()


SET(ENET_SOURCE
	callbacks.c
	eNetAdapt.h
	host.c
	list.c
	packet.c
	peer.c
	protocol.c
	)

IF(UNIX)
	SET(ENET_SOURCE ${ENET_SOURCE} unix.c)
ELSEIF(WIN32)
	SET(ENET_SOURCE ${ENET_SOURCE} win32.c)
ENDIF()

INCLUDE_DIRECTORIES(.)

ADD_LIBRARY(enet SHARED ${ENET_HEADERS} ${ENET_SOURCE})
