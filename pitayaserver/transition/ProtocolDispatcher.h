#ifndef __PROTOCOL_DISPATCHER_H__
#define __PROTOCOL_DISPATCHER_H__

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <map>
#include "WorldPacket.h"
#include "Defines.h"
#include "Log.h"

class HandlerBase
{
public:
	HandlerBase() {}
	virtual ~HandlerBase() {}

	virtual void Call( WorldPacket& pck ) = 0;
};

template<typename T /* ProtocolDataType */>
class Handler : public HandlerBase
{
	boost::function<void(T)> m_f;
public:
	Handler( boost::function<void(T)> f ):m_f(f)
	{
	}
	void Call( WorldPacket& pck )
	{
//		T data; pck >> data;
		T m;

		int remain = (int)pck.size();
		if (m.ParseFromArray((char*)pck.contents(), remain) == false)
		{
			IME_ERROR("parse data fail type %s error content %s", m.GetTypeName().c_str(), m.InitializationErrorString().c_str());
			return;
		}
		//std::string strPacket = m.Utf8DebugString();
		//IME_DEBUG("recv pkg %s", strPacket.c_str());
		google::protobuf::Descriptor const* descriptor = T::descriptor();
		std::string strPacket = m.ShortDebugString();
		if (strPacket.size() > 500)
			strPacket = strPacket.substr(0, 500) + " ...";
		IME_DEBUG("recv packet[%s](%u): %s", descriptor ? descriptor->name().c_str() : "null", m.cmd_id(), strPacket.c_str());

		if (!m.IsInitialized())
			IME_ERROR("recv message of type %s error content %s", m.GetTypeName().c_str(), m.InitializationErrorString().c_str());
		m_f( m );
	}
};

class ProtocolDispatcher
{
public:

	ProtocolDispatcher() {}
	virtual ~ProtocolDispatcher()
	{
		for ( HandlerContainer::iterator it = m_mapHandler.begin();
				it != m_mapHandler.end(); ++it )
		{
			delete it->second;
		}
		m_mapHandler.clear();
	}

	template<typename OBJ, typename MSG>
	bool AddHandler( OBJ* pObj, void (OBJ::*func)(MSG&) )
	{
		uint32 wCmdId = MSG::default_instance().cmd_id();
		if ( !m_mapHandler.insert(
				MK(
						wCmdId,
						new Handler<MSG>( boost::bind( func, pObj, _1 ) )
				) ).second )
		{
			IME_ERROR( "Duplicate CmdId, id=%u", wCmdId );
			return false;
		}
		return true;
	}

	int Dispatch( WorldPacket& pck )
	{
		HandlerContainer::iterator it = m_mapHandler.find( pck.GetOpcode() );
		if ( it != m_mapHandler.end() )
		{
//			IME_DEBUG("DisPatchProto, CmdId =%u",pck.GetOpcode());
			it->second->Call( pck );
			return 0;
		}
		else
		{
			return -1;
		}
	}

private:
	typedef std::map<uint32, HandlerBase*> HandlerContainer;
	HandlerContainer	m_mapHandler;
};



#endif
