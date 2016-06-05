
/* author : liuxu
* date : 2015.02.10
* description : memcache client api
*/

#include "CMemcache.h"

namespace NDBOpt
{
	CMemcache::CMemcache()
	{
		memc_ = memcached(NULL, 0);
	}

	CMemcache::CMemcache(const std::string &config)
	{
		memc_ = memcached(config.c_str(), config.size());
	}

	CMemcache::CMemcache(const std::string &hostname, in_port_t port)
	{
		memc_ = memcached(NULL, 0);
		if (memc_)
		{
			memcached_server_add(memc_, hostname.c_str(), port);
		}
	}

	CMemcache::CMemcache(memcached_st *clone)
	{
		memc_ = memcached_clone(NULL, clone);
	}

	CMemcache::CMemcache(const CMemcache &rhs)
	{
		memc_ = memcached_clone(NULL, rhs.getImpl());
	}

	CMemcache::~CMemcache()
	{
		memcached_free(memc_);
	}

	/**
	* Get the internal memcached_st *
	*/
	const memcached_st* CMemcache::getImpl() const
	{
		return memc_;
	}

	/**
	* Return an error string for the given return structure.
	*
	* @param[in] rc a memcached_return_t structure
	* @return error string corresponding to given return code in the library.
	*/
	const std::string CMemcache::getError(memcached_return_t rc) const
	{
		/* first parameter to strerror is unused */
		return memcached_strerror(NULL, rc);
	}

	bool CMemcache::error(std::string& error_message) const
	{
		if (memcached_failed(memcached_last_error(memc_)))
		{
			error_message += memcached_last_error_message(memc_);
			return true;
		}

		return false;
	}

	bool CMemcache::error() const
	{
		if (memcached_failed(memcached_last_error(memc_)))
		{
			return true;
		}

		return false;
	}

	bool CMemcache::error(memcached_return_t& arg) const
	{
		arg = memcached_last_error(memc_);
		return memcached_failed(arg);
	}

	bool CMemcache::setBehavior(memcached_behavior_t flag, uint64_t data)
	{
		return (memcached_success(memcached_behavior_set(memc_, flag, data)));
	}

	uint64_t CMemcache::getBehavior(memcached_behavior_t flag)
	{
		return memcached_behavior_get(memc_, flag);
	}

	/**
	* Configure the memcache object
	*
	* @param[in] in_config configuration
	* @return true on success; false otherwise
	*/
	bool CMemcache::configure(const std::string &configuration)
	{
		memcached_st *new_memc = memcached(configuration.c_str(), configuration.size());

		if (new_memc)
		{
			memcached_free(memc_);
			memc_ = new_memc;

			return true;
		}

		return false;
	}

	/**
	* Add a server to the list of memcached servers to use.
	*
	* @param[in] server_name name of the server to add
	* @param[in] port port number of server to add
	* @return true on success; false otherwise
	*/
	bool CMemcache::addServer(const std::string &server_name, in_port_t port)
	{
		return memcached_success(memcached_server_add(memc_, server_name.c_str(), port));
	}

	/**
	* Fetches an individual value from the server.
	*
	* @param[in] key key of object whose value to get
	* @param[out] ret_val object that is retrieved is stored in
	*                     this vector
	* @return true on success; false otherwise
	*/
	bool CMemcache::get(const std::string &key, std::vector<char> &ret_val, uint32_t &flags)
	{
		memcached_return_t rc;
		size_t value_length = 0;

		char *value = memcached_get(memc_, key.c_str(), key.length(),
			&value_length, &flags, &rc);
		if (value != NULL && ret_val.empty())
		{
			ret_val.reserve(value_length + 1); // Always provide null
			ret_val.assign(value, value + value_length + 1);
			ret_val.resize(value_length);
			free(value);

			return true;
		}

		return false;
	}

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
	bool CMemcache::getByKey(const std::string &master_key,
		const std::string &key,
		std::vector<char> &ret_val,
		uint32_t &flags)
	{
		memcached_return_t rc;
		size_t value_length = 0;

		char *value = memcached_get_by_key(memc_,
			master_key.c_str(), master_key.length(),
			key.c_str(), key.length(),
			&value_length, &flags, &rc);
		if (value)
		{
			ret_val.reserve(value_length + 1); // Always provide null
			ret_val.assign(value, value + value_length + 1);
			ret_val.resize(value_length);
			free(value);

			return true;
		}
		return false;
	}

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
	bool CMemcache::set(const std::string &key,
		const std::vector<char> &value,
		time_t expiration,
		uint32_t flags)
	{
		memcached_return_t rc = memcached_set(memc_,
			key.c_str(), key.length(),
			&value[0], value.size(),
			expiration, flags);
		return memcached_success(rc);
	}

	bool CMemcache::set(const std::string &key,
		const char* value, const size_t value_length,
		time_t expiration,
		uint32_t flags)
	{
		memcached_return_t rc = memcached_set(memc_,
			key.c_str(), key.length(),
			value, value_length,
			expiration, flags);
		return memcached_success(rc);
	}

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
	bool CMemcache::setByKey(const std::string& master_key,
		const std::string& key,
		const std::vector<char> &value,
		time_t expiration,
		uint32_t flags)
	{
		return memcached_success(memcached_set_by_key(memc_, master_key.c_str(),
			master_key.length(),
			key.c_str(), key.length(),
			&value[0], value.size(),
			expiration,
			flags));
	}


	/**
	* Add an object with the specified key and value to the server. This
	* function returns false if the object already exists on the server.
	*
	* @param[in] key key of object to add
	* @param[in] value of object to add
	* @return true on success; false otherwise
	*/
	bool CMemcache::add(const std::string& key, const std::vector<char>& value)
	{
		return memcached_success(memcached_add(memc_, key.c_str(), key.length(),
			&value[0], value.size(), 0, 0));
	}

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
	bool CMemcache::addByKey(const std::string& master_key,
		const std::string& key,
		const std::vector<char>& value)
	{
		return memcached_success(memcached_add_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			&value[0],
			value.size(),
			0, 0));
	}

	/**
	* Replaces an object on the server. This method only succeeds
	* if the object is already present on the server.
	*
	* @param[in] key key of object to replace
	* @param[in[ value value to replace object with
	* @return true on success; false otherwise
	*/
	bool CMemcache::replace(const std::string& key, const std::vector<char>& value)
	{
		return memcached_success(memcached_replace(memc_, key.c_str(), key.length(),
			&value[0], value.size(),
			0, 0));
	}

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
	bool CMemcache::replaceByKey(const std::string& master_key,
		const std::string& key,
		const std::vector<char>& value)
	{
		return memcached_success(memcached_replace_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			&value[0],
			value.size(),
			0, 0));
	}

	/**
	* Places a segment of data before the last piece of data stored.
	*
	* @param[in] key key of object whose value we will prepend data to
	* @param[in] value data to prepend to object's value
	* @return true on success; false otherwise
	*/
	bool CMemcache::prepend(const std::string& key, const std::vector<char>& value)
	{
		return memcached_success(memcached_prepend(memc_, key.c_str(), key.length(),
			&value[0], value.size(), 0, 0));
	}

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
	bool CMemcache::prependByKey(const std::string& master_key,
		const std::string& key,
		const std::vector<char>& value)
	{
		return memcached_success(memcached_prepend_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			&value[0],
			value.size(),
			0,
			0));
	}

	/**
	* Places a segment of data at the end of the last piece of data stored.
	*
	* @param[in] key key of object whose value we will append data to
	* @param[in] value data to append to object's value
	* @return true on success; false otherwise
	*/
	bool CMemcache::append(const std::string& key, const std::vector<char>& value)
	{
		return memcached_success(memcached_append(memc_,
			key.c_str(),
			key.length(),
			&value[0],
			value.size(),
			0, 0));
	}

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
	bool CMemcache::appendByKey(const std::string& master_key,
		const std::string& key,
		const std::vector<char> &value)
	{
		return memcached_success(memcached_append_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			&value[0],
			value.size(),
			0, 0));
	}

	/**
	* Delete an object from the server specified by the key given.
	*
	* @param[in] key key of object to delete
	* @return true on success; false otherwise
	*/
	bool CMemcache::remove(const std::string& key)
	{
		return memcached_success(memcached_delete(memc_, key.c_str(), key.length(), 0));
	}

	/**
	* Delete an object from the server specified by the key given.
	*
	* @param[in] key key of object to delete
	* @param[in] expiration time to delete the object after
	* @return true on success; false otherwise
	*/
	bool CMemcache::remove(const std::string& key, time_t expiration)
	{
		return memcached_success(memcached_delete(memc_,
			key.c_str(),
			key.length(),
			expiration));
	}

	/**
	* Delete an object from the server specified by the key given.
	*
	* @param[in] master_key specifies server to remove object from
	* @param[in] key key of object to delete
	* @return true on success; false otherwise
	*/
	bool CMemcache::removeByKey(const std::string& master_key,
		const std::string& key)
	{
		return memcached_success(memcached_delete_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			0));
	}

	/**
	* Delete an object from the server specified by the key given.
	*
	* @param[in] master_key specifies server to remove object from
	* @param[in] key key of object to delete
	* @param[in] expiration time to delete the object after
	* @return true on success; false otherwise
	*/
	bool CMemcache::removeByKey(const std::string& master_key,
		const std::string& key,
		time_t expiration)
	{
		return memcached_success(memcached_delete_by_key(memc_,
			master_key.c_str(),
			master_key.length(),
			key.c_str(),
			key.length(),
			expiration));
	}

	/**
	* Wipe the contents of memcached servers.
	*
	* @param[in] expiration time to wait until wiping contents of
	*                       memcached servers
	* @return true on success; false otherwise
	*/
	bool CMemcache::flush(time_t expiration)
	{
		return memcached_success(memcached_flush(memc_, expiration));
	}

	/**
	* Get the library version string.
	* @return std::string containing a copy of the library version string.
	*/
	const std::string CMemcache::libVersion() const
	{
		const char *ver = memcached_lib_version();
		const std::string version(ver);
		return version;
	}

	/**
	* Retrieve memcached statistics. Populate a std::map with the retrieved
	* stats. Each server will map to another std::map of the key:value stats.
	*
	* @param[out] stats_map a std::map to be populated with the memcached
	*                       stats
	* @return true on success; false otherwise
	*/
	bool CMemcache::getStats(std::map< std::string, std::map<std::string, std::string> >& stats_map)
	{
		memcached_return_t rc;
		memcached_stat_st *stats = memcached_stat(memc_, NULL, &rc);

		if (rc != MEMCACHED_SUCCESS &&
			rc != MEMCACHED_SOME_ERRORS)
		{
			return false;
		}

		uint32_t server_count = memcached_server_count(memc_);

		/*
		* For each memcached server, construct a std::map for its stats and add
		* it to the std::map of overall stats.
		*/
		for (uint32_t x = 0; x < server_count; x++)
		{
			const memcached_instance_st * instance = memcached_server_instance_by_position(memc_, x);
			std::ostringstream strstm;
			std::string server_name(memcached_server_name(instance));
			server_name.append(":");
			strstm << memcached_server_port(instance);
			server_name.append(strstm.str());

			std::map<std::string, std::string> server_stats;
			char **list = memcached_stat_get_keys(memc_, &stats[x], &rc);
			for (char** ptr = list; *ptr; ptr++)
			{
				char *value = memcached_stat_get_value(memc_, &stats[x], *ptr, &rc);
				server_stats[*ptr] = value;
				free(value);
			}

			stats_map[server_name] = server_stats;
			free(list);
		}

		memcached_stat_free(memc_, stats);
		return true;
	}
}