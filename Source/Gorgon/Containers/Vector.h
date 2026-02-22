#pragma once

#include <utility>

namespace Gorgon :: Containers {

	/// This function push_backs an item to the given vector
	/// if the item does not exists in the vector
	template<class T_, class V_>
	void PushBackUnique(V_ &vector, T_ &&val) {
		for(const auto &e : vector) {
			if(e == val) return;
		}

		vector.push_back(std::forward<T_>(val));
	}

	/// This function push_backs an item to the given vector
	/// if the item does not exists in the vector using the
	/// given predicate
	template<class T_, class V_, class P_>
	void PushBackUnique(V_ &vector, const T_ &val, P_ pred) {
		for(const auto &e : vector) {
			if(pred(e, val)) return;
		}

		vector.push_back(val);
	}
	
	/// This function push_backs an item to the given vector
	/// if the item does not exists in the vector, if the item
	/// is found, it will be updated
	template<class T_, class V_>
	void PushBackOrUpdate(V_ &vector, T_ &&val) {
		for(auto &e : vector) {
			if(e == val) e = std::forward<T_>(val);
		}

		vector.push_back(std::forward<T_>(val));
	}

	/// This function push_backs an item to the given vector
	/// if the item does not exists in the vector using the
	/// given predicate, if the item is found, it will be updated
	template<class T_, class V_, class P_>
	void PushBackOrUpdate(V_ &vector, const T_ &val, P_ pred) {
		for(auto &e : vector) {
			if(pred(e, val)) e = std::forward<T_>(val);
		}

		vector.push_back(val);
	}

}
