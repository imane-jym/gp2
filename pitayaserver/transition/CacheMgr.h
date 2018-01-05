#ifndef __CACHE_MGR_H__
#define __CACHE_MGR_H__

#include <tr1/memory>
//#include "Common.h"
#include "Defines.h"
#include <map>
#include <bitset>
#include "SqlOperation.h"
#include "Database.h"
#include "QueryResultMysql.h"
#include "Database.h"
#include "DatabaseImpl.h"

typedef uint32 CacheKeyType;

template<size_t S = 16> class CacheObject;
template<typename T, size_t S = 16> class CacheMgr;

template<size_t S>
class CacheObject
{
	template<typename U, size_t V>
	friend class CacheMgr;

public:
	CacheObject( CacheKeyType cacheId )
	:m_cacheId(cacheId),
	 m_useCount(0),
	 m_dirty(false),
	 m_inPersistence(false),
	 m_persistenceTime(0) {};
	virtual ~CacheObject() {};

	// load data from database
	virtual bool LoadData( uint32 filterIdx )
	{
		m_loadedDataFilter |= ( 1 << filterIdx );
		return true;
	}

	// serialize data to SQL, CacheMgr will save it to database later
	virtual bool SaveData( uint32 filterIdx, SqlQueryBatch* sqlBatch ) = 0;

	// whether exists in database or not
	virtual bool Exist( bool autoCreate ) = 0;

protected:
	CacheKeyType		m_cacheId;

private:

	std::bitset<S>		m_loadedDataFilter;
	uint32				m_useCount;
	bool				m_dirty;
	bool				m_inPersistence;
	time_t				m_persistenceTime;	// time when persistence complete
};

template <typename T, size_t S>
class CacheMgr
{
private:

	typedef std::map<CacheKeyType, T*> CacheContainer;
	CacheContainer		m_cacheMap;
	Database*			m_db;
	bool				m_dbUpdateAuto;
	uint32				m_persistenceInterval;
	uint32				m_removeFromCacheDelay;

public:

	typedef std::tr1::shared_ptr<T> CachePtr;

public:

	CacheMgr( Database* db, bool dbUpdateAuto ):m_db(db), m_dbUpdateAuto(dbUpdateAuto)
	{
		m_persistenceInterval	= sConfig->GetIntDefault( "game.persistence_interval", 180 );
		m_removeFromCacheDelay	= sConfig->GetIntDefault( "game.remove_from_cache_delay", 120 );
		IME_LOG( "CacheMgr Created" );
	};
	virtual ~CacheMgr()
	{
		ASSERT( m_cacheMap.empty() );
		IME_LOG( "CacheMgr Deleted" );
	};

	CachePtr Attach( CacheKeyType key , std::bitset<S> dataFilter, bool autoCreate = false )
	{
		T* data = NULL;
		typename CacheContainer::iterator it = m_cacheMap.find( key );

		if ( it != m_cacheMap.end() )
		{
			data = it->second;
		}
		else
		{
			data = new T( key );

			if ( !data->Exist( autoCreate ) )
			{
				IME_ERROR( "Attach Failed, autoCreate=%u", autoCreate );
				delete data;

				return CachePtr(); // NULL
			}
		}

		ASSERT( data );

		if ( !PrepareData( data, dataFilter ) )
		{
			IME_ERROR( "PrepareData Failed" );
			if ( it == m_cacheMap.end() ) delete data;

			return CachePtr();
		}

		data->m_persistenceTime = time(NULL);
		data->m_useCount++;

		m_cacheMap.insert( std::make_pair( key, data ) );


		IME_DEBUG( "Attach Succeeded, useCount=%u", data->m_useCount );

		return CachePtr( data, AutoRelease );
	};

	CachePtr FindInCache( CacheKeyType key )
	{
		typename CacheContainer::iterator it = m_cacheMap.find( key );
		if ( it != m_cacheMap.end() )
		{
			it->second->m_useCount++;
			IME_DEBUG( "FindCache Succeeded, useCount=%u", it->second->m_useCount );
			return CachePtr( it->second, AutoRelease );
		}
		return CachePtr();
	}

	void Detach( CachePtr& cache, bool modified )
	{
		if ( !cache ) return;

		// record something ?
		IME_DEBUG( "Detach" );

		cache.reset(); // call release
	}

	void Update()
	{
		if ( m_dbUpdateAuto ) m_db->ProcessResultQueue();

		typename CacheContainer::iterator it = m_cacheMap.begin();

		time_t cur = time(NULL);

		while ( it != m_cacheMap.end() )
		{
			typename CacheContainer::iterator nxt = it; ++nxt;

			T* data = it->second;

			if ( ( data->m_useCount > 0 || data->m_dirty ) &&
					!data->m_inPersistence &&
					cur > data->m_persistenceTime + m_persistenceInterval )
			{
				data->m_dirty = false;
				data->m_inPersistence = true;
				Persistence( data );
			}

			if ( data->m_useCount == 0 && !data->m_dirty &&
				!data->m_inPersistence && cur > data->m_persistenceTime + m_removeFromCacheDelay )
			{
				// we could remove it from cache at this moment
				IME_DEBUG( "Delete From Cache, id=%u", it->first );
				m_cacheMap.erase( it );
				delete data;
			}

			it = nxt;
		}
	}

	void Shutdown()
	{
		// make sure ASYNC persistence operation would be handled
		m_db->ProcessResultQueue();

		// other data in cache will persistence in SYNC way
		typename CacheContainer::iterator it = m_cacheMap.begin();

		while ( it != m_cacheMap.end() )
		{
			// no one should keep this data
			ASSERT( it->second->m_useCount == 0 );

			if ( it->second->m_dirty )
			{
				SqlQueryBatch batch;
				MakeSQL( it->second, &batch );
				batch.ExecuteDirect( m_db );

				IME_DEBUG( "Direct Persistence, id=%u", it->first );
			}

			delete it->second;
			++it;
		}

		m_cacheMap.clear();
	}

private:

	// T* will be deleted even if we forget to call Detach()
	static void AutoRelease( T* pCacheObj )
	{
		ASSERT( pCacheObj->m_useCount > 0 );

		pCacheObj->m_dirty = true;
		pCacheObj->m_useCount--;

		IME_DEBUG( "Release, useCount=%u", pCacheObj->m_useCount );
	}

	void OnPersistenceComp( QueryResult* result, SqlQueryBatch* batch, CacheKeyType cacheKey )
	{
		typename CacheContainer::iterator it = m_cacheMap.find( cacheKey );
		ASSERT( it != m_cacheMap.end() );

		IME_DEBUG( "Persistence Complete, id=%u", cacheKey );

		T* data = it->second;

		data->m_inPersistence = false;
		data->m_persistenceTime = time(NULL);

		// we keep it in cache until m_removeFromCacheDelay

		delete result;
		delete batch;
	}

	bool PrepareData( T* pCacheObj, const std::bitset<S>& dataFilter )
	{
		size_t sz = dataFilter.size();

		for ( size_t idx = 0; idx < dataFilter.size(); ++idx )
		{
			if ( dataFilter.test( idx ) && !pCacheObj->m_loadedDataFilter.test( idx ) )
			{
				if ( !pCacheObj->LoadData( idx ) )
				{
					pCacheObj->m_loadedDataFilter.reset( idx );
					return false;
				}

			}
		}
		return true;
	}

	void MakeSQL( T* pCacheObj, SqlQueryBatch* batch )
	{
		size_t sz = pCacheObj->m_loadedDataFilter.size();

		for ( size_t idx = 0; idx < sz; idx++ )
		{
			if ( pCacheObj->m_loadedDataFilter.test(idx) )
			{
				pCacheObj->SaveData( idx, batch );
			}
		}
	}

	void Persistence( T* pCacheObj )
	{
		SqlQueryBatch* batch = new SqlQueryBatch();
		// Serialize
		MakeSQL( pCacheObj, batch );
		// ASYNC Persistence
		m_db->DelayQueryBatch( this, &CacheMgr<T, S>::OnPersistenceComp, batch, pCacheObj->m_cacheId );

		IME_DEBUG( "Persistence Start, id=%u", pCacheObj->m_cacheId );
	}
};



#endif
