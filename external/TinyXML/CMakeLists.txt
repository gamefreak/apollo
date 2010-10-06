SET(TINYXML_HEADERS tinyxml.h)
SET(TINYXML_SOURCE 
	tinyxml.cpp
	tinyxmlerror.cpp
	tinyxmlparser.cpp
	)

ADD_LIBRARY(tinyxml SHARED ${TINYXML_HEADERS} ${TINYXML_SOURCE})
