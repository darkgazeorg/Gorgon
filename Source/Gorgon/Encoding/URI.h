 #pragma once

#include <stdexcept>
#include <set>
#include <map>
#include <vector>

namespace Gorgon :: Encoding {
	/// This error is thrown while URI decoding and building.
	class URIError : public std::runtime_error {
	public:
		using std::runtime_error::runtime_error;
	};

	/// Represents an unfolded URI. Can be used to parse a URI string or build one according to RFC 3986.
	/// @code
	/// #include <Gorgon/Network/HTTP.h>
	/// ...
	/// std::cout<<Gorgon::Network::HTTP::BlockingGetText(URI("http", "darkgaze.org", "path/to/data.php"))<<std::endl;
	/// @endcode
	class URI {
	public:
		/// Empty constructor. Does not set scheme thus would not be a valid URI
		URI() = default;

		/// Creates a new URI by filling in fields
		URI(const std::string &scheme, const std::string &host, const std::string &path, const std::string &query = "", const std::string &fragment="");

		/// Creates a new URI by filling in fields
		URI(const std::string &scheme, const std::string &host, int port, const std::string &path, const std::string &query = "", const std::string &fragment="");

		/// Builds a new URI from the given string. May throw URIError. This constructor will not check if the URI is 
		/// really valid. Use IsValid function for this purpose. If this URI is embedded inside another URI and possibly
		/// a relative link, set inpagelink to true. Otherwise, URIs without scheme will be treated having a host. 
		/// For instance when inpagelink is false, in URI darkgaze.org/my/path darkgaze.org would be treated as host
		/// name.
		URI(const std::string &str, bool inpagelink = false);

		/// Builds a new URI from the given string. May throw URIError. This constructor will not check if the URI is 
		/// really valid. Use IsValid function for this purpose.
		URI(const char *str) : URI(std::string(str)) { }

		/// Converts this URI to a properly encoded string
		operator std::string() const;

		/// Converts this URI to a properly encoded string
		std::string Convert() const { return *this; }

		/// Returns if this URI is valid. This function checks if scheme is valid in format. Additionally, if
		/// the scheme is http, https and ftp, checks if the host is set.
		bool IsValid() const;

		/// Returns if this URI is valid.
		operator bool() const {
			return IsValid();
		}

		/// Compares two URIs in their simplest forms. Note that URIs with different fragments point to
		/// different places in the same resource and will not be used in comparison. This is also
		/// recommended by RFC 3986
		bool operator ==(const URI &other) const;

		/// Compares two URIs in their simplest forms. Note that URIs with different fragments point to
		/// different places in the same resource and will not be used in comparison. This is also
		/// recommended by RFC 3986
		bool operator !=(const URI &other) const { return !(*this==other); }

		/// Combines a relative URI into this URI. Can be used to normalize links in a document.
		/// Care should be taken while using URIPath with this function as URIPath cannot represent
		/// relative paths.
		void Combine(const std::string &link);

		URI operator +(const std::string &link) { 
			auto t=*this; 
			t.Combine(link); 
			return t; 
		}

		/// Scheme of the URI, ex: http. Scheme cannot be encoded, thus should only start with an alpha character and 
		/// should only contain Alphanumeric characters as well as +, - and . 
		std::string scheme;

		/// Host address, either IP/domain name or empty, ex: darkgaze.org
		std::string host;

		/// User information. Use : to separate password. Use of password is no longer 
		/// recommended by RFC unless it is empty.
		std::string userinfo;

		/// The port number, 0 means it is not defined.
		int port = 0;

		/// Path of the resource. Segments must be separated by /. You may use URIPath to construct a path
		std::string path;

		/// Query for the resource, must be properly escaped and encoded, you may use HTTPQuery to build http query string.
		std::string query;

		/// Fragment of the resource (anchor in HTML)
		std::string fragment;
	};

	/// Represents and HTTP query that might be send to page using POST or embedded in URI
	class HTTPQuery {
	public:

		/// Empty constructor
		HTTPQuery() = default;

		/// Creates a new HTTP query using the given pairs
		HTTPQuery(std::initializer_list<std::pair<const std::string, std::string>> init);

		/// Parses the given query
		HTTPQuery(const std::string &query);

		/// Converts this query system to string
		operator std::string() const;

		/// Converts this query system to string
		std::string Convert() const { return *this; }
		
		/// Returns the value that is stored within key. If key does not exist, an empty string will be returned.
		std::string Get(const std::string &key) const {
            if(data.count(key) == 0) return "";
            
            return data.at(key);
        }
        
		/// Returns the value that is stored within key. If key does not exist, an empty value with the given key is created.
        std::string &operator[](const std::string &key) {
            return data[key];
        }
        
        /// Returns if the given key exists
        bool Exists(const std::string &key) const {
            return data.count(key) != 0;
        }

		/// Key-value pairs for this HTTP query
		std::map<std::string, std::string> data;
	};

	/// Helps to manage URIPaths. Note that the URI paths are always absolute. Allows normalization as 
	/// well as remapping, which would be useful to convert URI paths to file system paths.
	class URIPath {
	public:

		/// Empty constructor
		URIPath() = default;

		/// Initializes the path by the given segments
		URIPath(std::initializer_list<std::string> init);

		/// Parses the given path.
		URIPath(const std::string &path);

		/// Parses the given path.
		URIPath(const char *path) : URIPath(std::string(path)) { }

		/// Converts the path to string properly escaping for URI
		operator std::string() const;

		/// Compares two paths after normalization
		bool operator ==(const URIPath &other) const;

		/// Compares two paths after normalization
		bool operator !=(const URIPath &other) const { return !(*this==other); }
		
		int GetSize() const { return (int)segments.size(); }
		
		/// Returns the segment at the given index
		std::string &operator[] (int ind) { return segments[ind]; }
		
		/// Returns the segment at the given index. Index 0 will never cause an error, if does not exists, it will be empty string.
		std::string Get(int ind) const { 
            if(ind==0 && segments.size()==0) return "";
            
            return segments[ind]; 
        }
		
		/// Returns the first segment and removes it from the list of segments. This function will never fire error. If there are no segments
		/// it will return empty string
		std::string StripFirst() { 
            if(segments.size()==0) return "";
            
            auto ret = segments.front();
            segments.erase(segments.begin());
            
            return ret; 
        }
		
		/// Returns the segment at the given index
		const std::string &operator[] (int ind) const { return segments[ind]; }

		/// Converts the path to string properly escaping for URI
		std::string Convert() const { return *this; }

		/// Combines another URIPath using this one as the root
		void Combine(const URIPath &another);

		/// Combines this path with another, using this path as the root
		URIPath operator +(const URIPath &another) const { 
			auto newp = *this; 
			newp.Combine(another); 
			return newp;
		}

		/// Combines another URIPath into this one, using this path as the root
		URIPath &operator +=(const URIPath &another) {
			Combine(another);
			return *this;
		}

		/// Normalizes any relative references in the path. May throw URIError
		void Normalize();

		/// List of segments
		std::vector<std::string> segments;
	};

	std::ostream &operator <<(std::ostream &out, const URI &uri);

	std::ostream &operator <<(std::ostream &out, const HTTPQuery &query);

	std::ostream &operator <<(std::ostream &out, const URIPath &path);

	/// Decodes a percent-encoded URI string (RFC 3986).
	/// Set plusasspace=true for application/x-www-form-urlencoded input where '+' means space.
	std::string URIDecode(const std::string &str, bool plusasspace = false);

	/// Encodes a URI string, percent-encoding any character not in the unreserved set (RFC 3986).
	std::string URIEncode(const std::string &str);

	/// Customized percentage encoding. Some URI components have different characters that are allowed.
	/// Set spacetoplus=true for application/x-www-form-urlencoded encoding where space becomes '+'.
	std::string PCTEncode(const std::string &str, const std::set<char> &allowed, bool allowalpha=true, bool allownum=true, bool spacetoplus=false);
	
}
