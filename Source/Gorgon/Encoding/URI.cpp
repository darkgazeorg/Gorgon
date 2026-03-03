#include <string>
#include <cstring>
#include "URI.h"
#include "../String.h"

namespace Gorgon :: Encoding { 

	// Builds a 256-entry lookup table: ASCII char -> hex digit value, -1 for invalid chars.
	inline static char *buildhextodec() {
		auto ret = new char[256];

		std::memset(ret, -1, 256); // default: invalid

		for(int i=0; i<=9; i++)
			ret[i+'0']=i;			// '0'-'9'

		for(int i=10;i<16;i++)
			ret[i-10+'a']=ret[i-10+'A']=i; // 'a'-'f' and 'A'-'F'

		return ret;
	}

	// Builds a 256-entry lookup: true for RFC 3986 unreserved chars (alpha, digit, - . _ ~).
	inline static bool *buildsafelist() {
		auto ret = new bool[256];

		std::memset(ret, 0, 256);

		for(int i=0x41; i<=0x5a; i++) ret[i] = true; // A-Z
		for(int i=0x61; i<=0x7a; i++) ret[i] = true; // a-z
		for(int i=0x30; i<=0x39; i++) ret[i] = true; // 0-9

		// - . _ ~
		ret[0x2d] = true;
		ret[0x2e] = true;
		ret[0x5f] = true;
		ret[0x7e] = true;

		return ret;
	}

	static const     char *hextodec		= buildhextodec();
	static const     bool *safelist		= buildsafelist();
	static constexpr char  dectohex[]	= "0123456789ABCDEF";

	// Merge two character sets.
	static std::set<char> appendtoset(std::set<char> base, std::initializer_list<char> chars) {
		base.insert(chars.begin(), chars.end());

		return base;
	}

	static std::set<char> appendtoset(std::set<char> base, std::set<char> second) {
		base.insert(second.begin(), second.end());

		return base;
	}

	// RFC 3986 character class sets used for selective percent-encoding per component.
	static const     std::set<char> subdelims		= {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='};
	static const     std::set<char> unreservedchars	= {'-', '.', '_', '~'};
	static const     std::set<char> userinfochars	= appendtoset(appendtoset(subdelims, unreservedchars),
																  {':'}
													  );
	static const     std::set<char> hostchars		= appendtoset(appendtoset(subdelims, unreservedchars), 
																  {'[', ']', ':'}
													  );
	static const     std::set<char> pchar			= appendtoset(appendtoset(subdelims, unreservedchars), 
																  {':', '@'}
													  );
	static const     std::set<char> pathchars		= appendtoset(pchar, {'/'});
	static const     std::set<char> querychars		= appendtoset(pchar, {'?', '/'});

	// Decodes a percent-encoded URI string.
	// RFC 3986 does not treat '+' as space; set plusasspace=true only for
	// application/x-www-form-urlencoded (HTML query strings).
	std::string URIDecode(const std::string &src, bool plusasspace)	{
		std::string result;
		result.reserve(src.length());

		int pcte = 0;        // remaining hex digits expected after '%'
		unsigned char build = 0;
		for(auto c : src) {
			if(pcte) {
				// Use unsigned index; guard against signed-char platforms.
				auto digit = hextodec[(unsigned char)c];
				if(digit==char(-1)) {
					throw URIError(String::Concat("Non-hex character at URI: '", c, "'"));
				}

				build=build<<4|digit;

				if(--pcte==0) {
					result.push_back(build);
				}
			}
			else {
				if(c == '%')
					pcte=2;
				else if(plusasspace && c == '+') // form-encoded: '+' means space
					result.push_back(' ');
				else
					result.push_back(c);
			}
		}

		if(pcte) {
			throw URIError("Unterminated escape sequence in URI");
		}

		return result;
	}

	// Percent-encodes src, leaving allowed chars (and optionally alpha/digits) as-is.
	// set spacetoplus=true for application/x-www-form-urlencoded encoding of spaces.
	std::string PCTEncode(const std::string &src, const std::set<char> &allowed, bool allowalpha, bool allownum, bool spacetoplus) {
		std::string result;
		result.reserve(src.length());

		for(auto c : src) {
			unsigned char uc = (unsigned char)c;
			if(spacetoplus && c == ' ') {
				// form-encoded: space as '+'
				result.push_back('+');
			}
			else if(allowed.count(uc) || (isalpha(uc) && allowalpha) || (isdigit(uc) && allownum)) {
				result.push_back(c);
			}
			else {
				// percent-encode: use unsigned cast to avoid sign-extension on high-bit chars
				result.push_back('%');
				result.push_back(dectohex[uc >> 4]);
				result.push_back(dectohex[uc & 15]);
			}
		}

		return result;
	}

	std::string URIEncode(const std::string &src) {
		return PCTEncode(src, unreservedchars);
	}

	// Direct-construction from pre-known components; no encoding is applied.
	URI::URI(const std::string &scheme, const std::string &host,
			 const std::string &path,   const std::string &query, 
			 const std::string &fragment) : 
		scheme(scheme), host(host),
		path(path), query(query), fragment(fragment)
	{
	}

	// Same as above with explicit port.
	URI::URI(const std::string &scheme, const std::string &host, 
			 int port, const std::string &path, 
			 const std::string &query, const std::string &fragment) :
		scheme(scheme), host(host), port(port),
		path(path), query(query), fragment(fragment)
	{
	}

	URI::URI(const std::string &str, bool inpagelink) {
		auto it=str.begin();

		if(it==str.end()) return;

		bool hasscheme=true;

		//check if scheme exists
		if(str.find_first_of(':')==str.npos)
			hasscheme=false;
		else if(str.find_first_of(':')>str.find_first_of('/'))
			hasscheme=false;


		if(hasscheme) {
			//parse scheme
			while(it!=str.end()) {
				if(*it==':') {
					++it;
					break;
				}
				else {
					scheme.push_back(*it);
				}
				++it;
			}
		}

		if(it==str.end()) return;

		if(*it=='/' || (!hasscheme && !inpagelink)) {
			bool noslash=true;
			if(*it=='/') {
				it++;
				noslash=false;
			}
			if(it==str.end()) return;

			if(*it=='/' || noslash) {
				//parse authority

				if(*it=='/') it++;

				std::string temp; //we don't know if the string is userinfo or host
				bool doneuserinfo = false;
				bool doingport = false;
				bool doingipv6 = false;
				bool finishedipv6 = false;
				bool unknownipformat = false;
				bool first = true;
				bool mustbeuserinfo = false;

				while(it!=str.end()) {
					auto c = *it;
					if(first && c=='[') {
						doingipv6=true;

						if(it==str.end()) break;

						if(*(it+1)=='v') 
							unknownipformat=true;
					}
					else if(doingipv6) {
						if(c==']') {
							finishedipv6=true;
							++it;

							if(it==str.end()) return;

							c = *it;
							if(c==':')  {
								doingport=true;
								host=temp;
								temp="";
							}
							else if(c=='/' || c=='?' || c=='#') {
								host=temp;
								break;
							}
							else {
								throw URIError("Scrap after IPv6.");
							}
						}
						else {
							if(unknownipformat) {
								temp.push_back(c);
							}
							else if(c==':' || c=='.' || (c>='0' && c<='9') || (c>='a' && c<='f') || (c>='A' && c<='F')) {
								temp.push_back(c);
							}
							else {
								throw URIError("IPv6 format error");
							}
						}
					}
					else if(c=='/' || c=='?' || c=='#') {
						if(mustbeuserinfo) {
							throw URIError("Port number should be numeric");
						}

						if(doingport) {
							port=String::To<int>(temp);
						}
						else {
							host=temp;
						}

						break;
					}
					else if(doingport) {
						if(isdigit((unsigned char)c)) {
							temp.push_back(c);
						}
						else {
							if(doneuserinfo)
								throw URIError("Port number should be numeric");
							else { // could be userinfo, which can have : in it.
								temp=host+":"+temp;
								doingport=false;
								mustbeuserinfo=true;

								continue;
							}
						}
					}
					else if(c == '@') { //previously collected data is userinfo
						if(doneuserinfo)
							throw URIError("Multiple userinfo in authority");

						userinfo=temp;
						doneuserinfo=true;
						mustbeuserinfo=false;
						temp="";
					}
					else if(c==':' && !mustbeuserinfo) {
						doingport=true;
						host=temp;
						temp="";
					}
					else {
						temp.push_back(c);
					}

					first=false;
					++it;
				}

				if(doingipv6 && !finishedipv6) {
					throw URIError("IPv6 is not finished with ]");
				}

				if(temp!="") {
					if(doingport) {
						port=String::To<int>(temp);
					}
					else {
						host=temp;
					}
				}

			} //parse authority
			else {
				--it; // '/' is for path
			}

		}
		//parse path
		while(it!=str.end()) {
			auto c=*it;

			if(c=='#' || c=='?') {
				break;
			}
			else {
				path.push_back(c);
			}

			++it;
		}

		if(it==str.end()) return;

		//parse query
		if(*it=='?') {
			++it;

			while(it!=str.end()) {
				auto c=*it;

				if(c=='#')
					break;
				else
					query.push_back(c);

				++it;
			}
		}

		if(it==str.end()) return;

		//parse fragment
		//this part will only execute if *it is #
		while(++it!=str.end())
			fragment.push_back(*it);
	}

	URI::operator std::string() const {
		std::string ret;
		ret = String::ToLower(scheme)+":";
		
		//authority
		if(host!="" || userinfo!="") {
			ret+="//";

			if(userinfo!="")
				ret+=PCTEncode(userinfo, userinfochars)+"@";

			ret+=PCTEncode(String::ToLower(host), hostchars);

			if(port!=0)
				ret+=":"+std::to_string(port);
		}

		if(path!="") {
			if(path[0]!='/' && (host!="" || userinfo!="")) {
				ret+="/./"+PCTEncode(path, pathchars);
			}
			else {
				ret+=PCTEncode(path, pathchars);
			}
		}

		if(query!="")
			ret+="?"+PCTEncode(query, querychars);

		if(fragment!="")
			ret+="#"+PCTEncode(fragment, querychars);

		return ret;
	}

	bool URI::IsValid() const {
		// Scheme must start with alpha and contain only alpha/digit/+/-/.
		if(scheme.length()==0)  return false;

		if(!isalpha((unsigned char)scheme[0])) return false;

		if(PCTEncode(scheme, {'+', '-', '.'})!=scheme) return false;

		// These schemes require a host.
		if(scheme=="http" || scheme=="https" || scheme=="ftp")
			if(host=="") return false;

		return true;
	}

	void URI::Combine(const std::string &link) {
		if(link=="") return;

		auto uriform = URI(link, true);

		if(uriform.scheme!="") {
			scheme=uriform.scheme;
			host=uriform.host;
			path=uriform.path;
			query=uriform.query;
			fragment=uriform.fragment;
		}
		else if(uriform.host!="") {
			host=uriform.host;
			path=uriform.path;
			query=uriform.query;
			fragment=uriform.fragment;
		}
		else {
			fragment=uriform.fragment;

			if(uriform.path=="") {
				if(uriform.query!="")
					query=uriform.query;
			}
			else {
				bool relative = true;

				//if starts with / but not with /./ and /../ then the path is absolute
				if(uriform.path[0]=='/') { 
					if(uriform.path.length()<2)
						relative = false;
					else if(uriform.path[1]!='.') // unless first / is followed by . it is absolute
						relative = false;
					else if(uriform.path.length()>=3) { // /. but
						if(uriform.path[2] != '/') { //not /./
							if(uriform.path[2] != '.') { // and not /.. either
								relative = false;
							}
							else  // /..
								if(uriform.path.length()>=4 && uriform.path[3]!='/') // not /../
									relative=false;
						}
					}
				}

				if(relative) {
					// Append relative path then normalize away . and .. segments
					auto newpath = URIPath(path) + URIPath(uriform.path);
					newpath.Normalize();
					path  = newpath;
					query = uriform.query;
				}
				else {
					// Absolute path - still normalize . and .. segments
					auto newpath = URIPath(uriform.path);
					newpath.Normalize();
					path  = newpath;
					query = uriform.query;
				}
			}
		}
	}

	// Fragment is intentionally excluded from comparison (RFC 3986 §6.2).
	bool URI::operator==(const URI &other) const {
		if(String::ToLower(scheme) != String::ToLower(other.scheme)) return false;

		if(userinfo!=other.userinfo) return false;

		// RFC: file://localhost/ == file:///
		if(scheme == "file" && ( 
			(host=="" && String::ToLower(other.host)=="localhost") || 
			(other.host=="" && String::ToLower(host)=="localhost") )) {
			; //no problem, in file scheme localhost == ""
		}
		else  if(String::ToLower(host) != String::ToLower(other.host)) return false;

		// Expand default ports so port 0 compares correctly against explicit defaults.
		int thisport = port;
		int otherport = other.port;

		if(scheme=="http") {
			if(thisport==0) thisport=80;
			if(otherport==0) otherport=80;
		}
		else if(scheme=="https") {
			if(thisport==0) thisport=443;
			if(otherport==0) otherport=443;
		}
		else if(scheme=="ftp") {
			if(thisport==0) thisport=21;
			if(otherport==0) otherport=21;
		}
		else if(scheme=="sftp" || scheme=="ssh") {
			if(thisport==0) thisport=22;
			if(otherport==0) otherport=22;
		}

		if(thisport!=otherport) return false;

		if(URIPath(path) != URIPath(other.path)) return false;

		return query==other.query;
	}

	HTTPQuery::HTTPQuery(std::initializer_list<std::pair<const std::string, std::string>> init) :
		data(std::move(init))
	{ }

	// Serialises to application/x-www-form-urlencoded format (RFC 1866):
	// spaces become '+', everything else is percent-encoded.
	HTTPQuery::operator std::string() const {
		std::string ret;

		for(auto p : data) {
			if(ret!="")
				ret+="&";

			// Encode key and value: space -> '+', other special chars -> %XX
			ret += PCTEncode(String::FixLineEndings(p.first),  {}, true, true, true) + "=";
			ret += PCTEncode(String::FixLineEndings(p.second), {}, true, true, true);
		}

		return ret;
	}

	// Parses an application/x-www-form-urlencoded query string.
	// '+' is treated as space per RFC 1866; percent-sequences are decoded.
	HTTPQuery::HTTPQuery(const std::string &query) {
		std::string temp;
		bool invalue = false;
		std::string key;

		for(auto c : query) {
			if(c=='&' || c==';') {
				// end of pair; flush key if we never saw '='
				if(!invalue) {
					key = URIDecode(temp, true);
					temp="";
				}

				data.insert({key, URIDecode(temp, true)});

				temp="";
				invalue=false;
			}
			else if(!invalue && c=='=') {
				// separator between key and value
				key = URIDecode(temp, true);
				temp="";
				invalue=true;
			}
			else {
				temp.push_back(c);
			}
		}

		// flush last pair
		if(temp!="" || key!="") {
			if(!invalue)
				key = URIDecode(temp, true);

			data.insert({key, URIDecode(temp, true)});
		}
	}

	URIPath::URIPath(std::initializer_list<std::string> init) : 
		segments(std::move(init))
	{ }

	// Parse a path string into segments, ignoring the leading '/'.
	URIPath::URIPath(const std::string &path) {
		std::string temp;
		bool first=true;

		for(auto c : path) {
			if(c=='/') {
				if(!first)
					segments.push_back(temp); // push segment between slashes

				temp="";
			}
			else
				temp.push_back(c);

			first=false;
		}

		if(temp!="")      // last segment has no trailing '/'
			segments.push_back(temp);
	}

	// Resolves '.' (current) and '..' (parent) segments in-place.
	void URIPath::Normalize() {
		for(int i = 0; i<(int)segments.size(); i++) {
			auto &seg = segments[i];

			if(seg==".") {
				// drop current-directory reference
				segments.erase(segments.begin()+i);
				i--;
			}
			else if(seg=="..") {
				if(i==0) {
					throw URIError("Parent directory directive at the top segment.");
				}

				// remove parent and '..' together
				segments.erase(segments.begin()+i-1, segments.begin()+i+1);
				i-=2;
			}
		}
	}

	URIPath::operator std::string() const {
		std::string ret;

		for(auto seg : segments) {
			ret+="/"+seg;
		}

		return ret;
	}

	void URIPath::Combine(const URIPath &another) {
		segments.reserve(segments.size()+another.segments.size());

		for(auto seg : another.segments)
			segments.push_back(seg);
	}

	bool URIPath::operator==(const URIPath &other) const {
		auto t1=*this;
		auto t2=other;

		t1.Normalize();
		t2.Normalize();
		
		if(t1.segments != t2.segments)
			return false;		

		return true;
	}

	std::ostream &operator <<(std::ostream &out, const URI &uri) {
		out<<uri.Convert();

		return out;
	}

	std::ostream &operator <<(std::ostream &out, const HTTPQuery &query) {
		for(auto p : query.data) {
			out<<p.first<<" = "<<p.second<<std::endl;
		}

		return out;
	}

	std::ostream &operator <<(std::ostream &out, const URIPath &path) {
		out<<path.Convert();

		return out;
	}

}
