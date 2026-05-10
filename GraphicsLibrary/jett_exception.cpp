//
//  Jett Graphics Library
//
//  Copyright (c) 2026 Matt Pyne. All rights reserved.
//

#include "StdAfx.h"
#include "jett.h"


jett_exception::jett_exception( int code, int subsys_code, const char* message )
{
    m_code = code;
    m_subsys_code = subsys_code;
	size_t len = strlen( message );
	m_message = new char[ len + 1];
#ifdef _WIN32
	strncpy_s( m_message, len+1, message, len );
#else
	strncpy( m_message, message, len );
	m_message[len] = 0;
#endif
}

jett_exception::jett_exception( const jett_exception& b )
{
    *this = b;
}

jett_exception& jett_exception::operator=( const jett_exception &b )
{
	m_code = b.m_code;
    m_subsys_code = b.m_subsys_code;
	int len = strlen( b.m_message );
	m_message = new char[ len + 1];
#ifdef _WIN32
	strncpy_s( m_message, len+1, b.m_message, len );
#else
	strncpy( m_message, b.m_message, len );
	m_message[len] = 0;
#endif

	return *this;
}

jett_exception::~jett_exception()
{
	delete [] m_message;
}
