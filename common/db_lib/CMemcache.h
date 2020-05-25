
/* author : admin
 * date : 2015.02.10
 * description :  memcache client api
 */
 
#ifndef __MEMCACHE_H__
#define __MEMCACHE_H__

#include <libmemcached/memcached.h>

#include <string.h>

#include <sstream>
#include <string>
#include <vector>
#include <map>

namespace NDBOpt
{

	/**
	* This is the core memcached library (if later, other objects
	* are needed, they will be created from this class).
	*/
	class CMemcache
	{
	public:

		CMemcache();
		CMemcache(const std::string &config);
		CMemcache(const std::string &hostname, in_port_t port);
		CMemcache(memcached_st *clone);
		CMemcache(const CMemcache &rhs);

		~CMemcache();

		/**
		* Get the internal memcached_st *
		*/
		const memcached_st *getImpl() const;

		/**
		* Return an error string for the given return structure.
		*
		* @param[in] rc a memcached_return_t structure
		* @return error string corresponding to given return code in the library.
		*/
		const std::string getError(memcached_return_t rc) const;
		bool error(std::string& error_message) const;
		bool error() const;
		bool error(memcached_return_t& arg) const;
		bool setBehavior(memcached_behavior_t flag, uint64_t data);
		uint64_t getBehavior(memcached_behavior_t flag);

		/**
		* Configure the memcache object
		*
		* @param[in] in_config configuration
		* @return true on success; false otherwise
		*/
		bool configure(const std::string &configuration);

		/**
		* Add a server to the list of memcached servers to use.
		*
		* @param[in] server_name name of the server to add
		* @param[in] port port number of server to add
		* @return true on success; false otherwise
		*/
		bool addServer(const std::string &server_name, in_port_t port);

		/**
		* Fetches an individual value from the server.
		*
		* @param[in] key key of object whose value to get
		* @param[out] ret_val object that is retrieved is stored in
		*                     this vector
		* @return true on success; false otherwise
		*/
		bool get(const std::string &key, std::vector<char> &ret_val, uint32_t &flags);

		/**
		* Fetches an individual from a server which is specified by
		* the master_key parameter that is used for determining which
		* server an object was stored in if key partitioning was
		* used for storage.
		*
		* @param[in] master_key key that specifies server object is stored on
		* @param[in] key key of object whose value to get
		* @param[out] ret_val object that is retrieved is stored in
		*                     this vector
		* @return true on success; false otherwise
		*/
		bool getByKey(const std::string &master_key,
			const std::string &key,
			std::vector<char> &ret_val,
			uint32_t &flags);

		/**
		* Writes an object to the server. If the object already exists, it will
		* overwrite the existing object. This method always returns true
		* when using non-blocking mode unless a network error occurs.
		*
		* @param[in] key key of object to write to server
		* @param[in] value value of object to write to server
		* @param[in] expiration time to keep the object stored in the server for
		* @param[in] flags flags to store with the object
		* @return true on succcess; false otherwise
		*/
		bool set(const std::string &key,
			const std::vector<char> &value,
			time_t expiration,
			uint32_t flags);

		bool set(const std::string &key,
			const char* value, const size_t value_length,
			time_t expiration,
			uint32_t flags);

		/**
		* Writes an object to a server specified by the master_key parameter.
		* If the object already exists, it will overwrite the existing object.
		*
		* @param[in] master_key key that specifies server to write to
		* @param[in] key key of object to write to server
		* @param[in] value value of object to write to server
		* @param[in] expiration time to keep the object stored in the server for
		* @param[in] flags flags to store with the object
		* @return true on succcess; false otherwise
		*/
		bool setByKey(const std::string& master_key,
			const std::string& key,
			const std::vector<char> &value,
			time_t expiration,
			uint32_t flags);

		/**
		* Add an object with the specified key and value to the server. This
		* function returns false if the object already exists on the server.
		*
		* @param[in] key key of object to add
		* @param[in] value of object to add
		* @return true on success; false otherwise
		*/
		bool add(const std::string& key, const std::vector<char>& value);

		/**
		* Add an object with the specified key and value to the server. This
		* function returns false if the object already exists on the server. The
		* server to add the object to is specified by the master_key parameter.
		*
		* @param[in[ master_key key of server to add object to
		* @param[in] key key of object to add
		* @param[in] value of object to add
		* @return true on success; false otherwise
		*/
		bool addByKey(const std::string& master_key,
			const std::string& key,
			const std::vector<char>& value);

		/**
		* Replaces an object on the server. This method only succeeds
		* if the object is already present on the server.
		*
		* @param[in] key key of object to replace
		* @param[in[ value value to replace object with
		* @return true on success; false otherwise
		*/
		bool replace(const std::string& key, const std::vector<char>& value);

		/**
		* Replaces an object on the server. This method only succeeds
		* if the object is already present on the server. The server
		* to replace the object on is specified by the master_key param.
		*
		* @param[in] master_key key of server to replace object on
		* @param[in] key key of object to replace
		* @param[in[ value value to replace object with
		* @return true on success; false otherwise
		*/
		bool replaceByKey(const std::string& master_key,
			const std::string& key,
			const std::vector<char>& value);

		/**
		* Places a segment of data before the last piece of data stored.
		*
		* @param[in] key key of object whose value we will prepend data to
		* @param[in] value data to prepend to object's value
		* @return true on success; false otherwise
		*/
		bool prepend(const std::string& key, const std::vector<char>& value);

		/**
		* Places a segment of data before the last piece of data stored. The
		* server on which the object where we will be prepending data is stored
		* on is specified by the master_key parameter.
		*
		* @param[in] master_key key of server where object is stored
		* @param[in] key key of object whose value we will prepend data to
		* @param[in] value data to prepend to object's value
		* @return true on success; false otherwise
		*/
		bool prependByKey(const std::string& master_key,
			const std::string& key,
			const std::vector<char>& value);

		/**
		* Places a segment of data at the end of the last piece of data stored.
		*
		* @param[in] key key of object whose value we will append data to
		* @param[in] value data to append to object's value
		* @return true on success; false otherwise
		*/
		bool append(const std::string& key, const std::vector<char>& value);

		/**
		* Places a segment of data at the end of the last piece of data stored. The
		* server on which the object where we will be appending data is stored
		* on is specified by the master_key parameter.
		*
		* @param[in] master_key key of server where object is stored
		* @param[in] key key of object whose value we will append data to
		* @param[in] value data to append to object's value
		* @return true on success; false otherwise
		*/
		bool appendByKey(const std::string& master_key,
			const std::string& key,
			const std::vector<char> &value);

		/**
		* Delete an object from the server specified by the key given.
		*
		* @param[in] key key of object to delete
		* @return true on success; false otherwise
		*/
		bool remove(const std::string& key);

		/**
		* Delete an object from the server specified by the key given.
		*
		* @param[in] key key of object to delete
		* @param[in] expiration time to delete the object after
		* @return true on success; false otherwise
		*/
		bool remove(const std::string& key, time_t expiration);

		/**
		* Delete an object from the server specified by the key given.
		*
		* @param[in] master_key specifies server to remove object from
		* @param[in] key key of object to delete
		* @return true on success; false otherwise
		*/
		bool removeByKey(const std::string& master_key,
			const std::string& key);

		/**
		* Delete an object from the server specified by the key given.
		*
		* @param[in] master_key specifies server to remove object from
		* @param[in] key key of object to delete
		* @param[in] expiration time to delete the object after
		* @return true on success; false otherwise
		*/
		bool removeByKey(const std::string& master_key,
			const std::string& key,
			time_t expiration);

		/**
		* Wipe the contents of memcached servers.
		*
		* @param[in] expiration time to wait until wiping contents of
		*                       memcached servers
		* @return true on success; false otherwise
		*/
		bool flush(time_t expiration = 0);

		/**
		* Get the library version string.
		* @return std::string containing a copy of the library version string.
		*/
		const std::string libVersion() const;

		/**
		* Retrieve memcached statistics. Populate a std::map with the retrieved
		* stats. Each server will map to another std::map of the key:value stats.
		*
		* @param[out] stats_map a std::map to be populated with the memcached
		*                       stats
		* @return true on success; false otherwise
		*/
		bool getStats(std::map< std::string, std::map<std::string, std::string> >& stats_map);

	private:
		CMemcache &operator=(const CMemcache &rhs);

	private:
		memcached_st *memc_;
	};

}

#endif // MEMCACHE_H


