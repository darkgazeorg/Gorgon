#include <string>
#include <array>
#include "URI.h"
#include "../String.h"

namespace Gorgon :: Encoding { 

    // Lookup table: ASCII char -> hex digit value (0-15), or -1 for non-hex chars.
    static constexpr std::array<int, 256> buildhextodec() noexcept {
        std::array<int, 256> ret{};
        for(auto &v : ret) v = -1;         // default: invalid
        for(int i=0; i<=9; i++)
            ret['0'+i] = i;               // '0'-'9'
        for(int i=10; i<16; i++) {
            ret['a'+i-10] = i;            // 'a'-'f'
            ret['A'+i-10] = i;            // 'A'-'F'
        }
        return ret;
    }

    static constexpr auto  hextodec        = buildhextodec();
    static constexpr char  dectohex[]    = "0123456789ABCDEF";

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
    static const     std::set<char> subdelims        = {'!', '$', '&', '\'', '(', ')', '*', '+', ',', ';', '='};
    static const     std::set<char> unreservedchars    = {'-', '.', '_', '~'};
    static const     std::set<char> userinfochars    = appendtoset(appendtoset(subdelims, unreservedchars),
                                                                  {':'}
                                                      );
    static const     std::set<char> hostchars        = appendtoset(appendtoset(subdelims, unreservedchars), 
                                                                  {'[', ']', ':'}
                                                      );
    static const     std::set<char> pchar            = appendtoset(appendtoset(subdelims, unreservedchars), 
                                                                  {':', '@'}
                                                      );
    static const     std::set<char> pathchars        = appendtoset(pchar, {'/'});
    static const     std::set<char> querychars       = appendtoset(pchar, {'?', '/'});

    // Decodes a percent-encoded URI string.
	// RFC 3986 requires two hex digits after '%'; invalid sequences make the URI
	// malformed. When strict==false the decoder performs best-effort parsing and
	// leaves any malformed escape sequence in the output verbatim.
	std::string URIDecode(const std::string &src, bool plusasspace, bool strict) {
		std::string result;
		result.reserve(src.length());
		size_t i = 0;
		auto len = src.size();
		while(i < len) {
			char c = src[i];
			if(c == '%') {
				if(i + 2 >= len) {
					if(strict) {
						throw URIError("Unterminated escape sequence in URI");
					}
					// copy literal percent and whatever follows
					result.push_back('%');
					if(i + 1 < len) result.push_back(src[i+1]);
					i += 1 + (i + 1 < len);
					continue;
				}
				char c1 = src[i+1];
				char c2 = src[i+2];
				int d1 = hextodec[(unsigned char)c1];
				int d2 = hextodec[(unsigned char)c2];
				if(d1 == -1 || d2 == -1) {
					if(strict) {
						throw URIError(String::Concat("Non-hex character at URI: '",
							(d1==-1?c1:c2), "'"));
					}
					// best-effort: emit '%' and advance one character
					result.push_back('%');
					i++;
					continue;
				}
				result.push_back((char)((d1<<4) | d2));
				i += 3;
			}
			else if(plusasspace && c == '+') {
				result.push_back(' ');
				i++;
			}
			else {
				result.push_back(c);
				i++;
			}
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

    // Returns the effective port: the stored port if set, otherwise the well-known
    // default for the scheme, or 0 if unknown.
    int URI::GetPortNumber() const {
        if(port != 0) return port;

        auto s = String::ToLower(scheme);
        if(s == "http")                 return 80;
        if(s == "https")                return 443;
        if(s == "ftp")                  return 21;
        if(s == "sftp" || s == "ssh")   return 22;
        if(s == "ldap")                 return 389;
        if(s == "ldaps")                return 636;
        if(s == "smtp")                 return 25;
        if(s == "smtps")                return 465;
        if(s == "imap")                 return 143;
        if(s == "imaps")                return 993;
        if(s == "pop3")                 return 110;
        if(s == "pop3s")                return 995;
        if(s == "telnet")               return 23;

        return 0; // no known default
    }

    // Normalize this URI in-place (RFC 3986 §6.2.2):
    //   - lowercase scheme and host
    //   - resolve . and .. in path
    // Port number defaults are NOT applied here; use GetPortNumber() or the == operator.
    void URI::Normalize(bool removeempty) {
        scheme = String::ToLower(scheme);
        host   = String::ToLower(host);

        auto p = URIPath(path);
        p.Normalize(removeempty);
        path = p;
    }

    void URI::Combine(const std::string &link) {
        if(link=="") return;

        auto uriform = URI(link, true);

        if(uriform.scheme!="") {
            scheme=uriform.scheme;
			port=uriform.port;
			userinfo=uriform.userinfo;
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
        if(String::ToLower(scheme) == "file" && ( 
            (host=="" && String::ToLower(other.host)=="localhost") || 
            (other.host=="" && String::ToLower(host)=="localhost") )) {
            ; //no problem, in file scheme localhost == ""
        }
        else  if(String::ToLower(host) != String::ToLower(other.host)) return false;

        // Expand default ports so port 0 compares correctly against explicit defaults.
        int thisport = port;
        int otherport = other.port;

        if(String::ToLower(scheme)=="http") {
            if(thisport==0) thisport=80;
            if(otherport==0) otherport=80;
        }
        else if(String::ToLower(scheme)=="https") {
            if(thisport==0) thisport=443;
            if(otherport==0) otherport=443;
        }
        else if(String::ToLower(scheme)=="ftp") {
            if(thisport==0) thisport=21;
            if(otherport==0) otherport=21;
        }
        else if(String::ToLower(scheme)=="sftp" || String::ToLower(scheme)=="ssh") {
            if(thisport==0) thisport=22;
            if(otherport==0) otherport=22;
        }

        if(thisport!=otherport) return false;

        if(URIPath(path) != URIPath(other.path)) return false;

        return query==other.query;
    }

    HTTPQuery::HTTPQuery(std::initializer_list<HTTPQuery::KeyPair> init) :
        data(init)
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

    // Returns the last value for key, or empty string if not found.

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
                    key = URIDecode(temp, true, true);
                    temp="";
                }

                data.push_back({key, URIDecode(temp, true, true)});

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
                key = URIDecode(temp, true, true);

            data.push_back({key, URIDecode(temp, true, true)});
        }
    }

    URIPath::URIPath(std::initializer_list<std::string> init) : 
        segments(std::move(init))
    { }

    // Parse a path string into segments, ignoring the leading '/'.
    URIPath::URIPath(const std::string &path, bool strict) {
        std::string temp;
        bool first=true;

        for(auto c : path) {
            if(c=='/') {
                if(!first)
                    segments.push_back(URIDecode(temp, false, strict)); // push segment between slashes

                temp="";
            }
            else
                temp.push_back(c);

            first=false;
        }

        if(temp!="")      // last segment has no trailing '/'
            segments.push_back(URIDecode(temp, false, strict));
    }

    // Resolves '.' (current), '..' (parent), and empty segments in-place.
    // RFC 3986 §3.3 permits empty segments but §6.2.2 allows implementations to remove them.
    void URIPath::Normalize(bool removeempty) {
        for(int i = 0; i<(int)segments.size(); i++) {
            auto &seg = segments[i];

            if((removeempty && seg=="") || seg==".") {
                // drop empty segment (from //) or current-directory reference
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

    // Returns true if this path can be safely normalized (no '..' past root).
    bool URIPath::IsValid() const {
        int depth = 0;
        for(auto &seg : segments) {
            if(seg == "..") {
                if(depth == 0) return false; // would pop past root
                depth--;
            }
            else if(seg != "." && seg != "") {
                depth++;
            }
        }
        return true;
    }

    URIPath::operator std::string() const {
        std::string ret;

        for(auto seg : segments) {
            ret+="/"+URIEncode(seg);
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
