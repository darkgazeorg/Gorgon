#pragma once

#include "Base.h"
#include "Reader.h"
#include "DataItems.h"
#include "../TMP.h"


namespace Gorgon :: Resource {
	
	class File;
	
	class Data : public Base {

		template <class T_, class R_, int IND_>
		void append(const T_ &values, const std::string &prefix, const R_ &reflectionobj) {
			Append(prefix+reflectionobj.Names[IND_], values.*(R_::template Member<IND_>::MemberPointer()) );
		}


		template <class T_, class R_, int ...S_>
		void append(const T_ &values, const std::string &prefix, const R_ &reflectionobj, TMP::Sequence<S_...>) {
			char dummy[] ={0, (append<T_, R_, S_>(values, prefix, reflectionobj),'\0')...};
		}

		template <class T_, class O_, class R_, int IND_>
		typename std::enable_if<!std::is_base_of<Base, typename std::remove_pointer<O_>::type>::value, void>::type
		namedtransform(T_ &values, const std::string &prefix, const R_ &reflectionobj) const {
			values.*(R_::template Member<IND_>::MemberPointer())=GetItem(prefix+reflectionobj.Names[IND_]).template Get<O_>();
		}

		template <class T_, class R_, int ...S_>
		void namedtransform(T_ &values, const std::string &prefix, const R_ &reflectionobj, TMP::Sequence<S_...>) const {
			char dummy[] ={0, (namedtransform<T_, typename R_::template Member<S_>::Type, R_, S_>(values, prefix, reflectionobj),'\0')...};
		}

		template <class T_, class O_, class R_, int IND_>
		typename std::enable_if<std::is_base_of<Base, typename std::remove_pointer<O_>::type>::value, void>::type
		namedtransform(T_ &values, const std::string &prefix, const R_ &reflectionobj) {
			values.*(R_::template Member<IND_>::MemberPointer())=&GetItem(prefix+reflectionobj.Names[IND_]).template GetObject<typename std::remove_pointer<O_>::type>();
			dynamic_cast<ObjectData&>(GetItem(prefix+reflectionobj.Names[IND_])).Release();
		}

		template <class T_, class O_, class R_, int IND_>
		typename std::enable_if<!std::is_base_of<Base, typename std::remove_pointer<O_>::type>::value, void>::type
		namedtransform(T_ &values, const std::string &prefix, const R_ &reflectionobj) {
			values.*(R_::template Member<IND_>::MemberPointer())=GetItem(prefix+reflectionobj.Names[IND_]).template Get<O_>();
		}

		template <class T_, class R_, int ...S_>
		void namedtransform(T_ &values, const std::string &prefix, const R_ &reflectionobj, TMP::Sequence<S_...>) {
			char dummy[] ={0, (namedtransform<T_, typename R_::template Member<S_>::Type, R_, S_>(values, prefix, reflectionobj),'\0')...};
		}

	public:
		/// Iterator for the data resource
		using Iterator = Containers::Collection<DataItem>::Iterator;

		/// Constant iterator for the data resource
		using ConstIterator = Containers::Collection<DataItem>::ConstIterator;

		/// Creates an empty Data
		Data() { }
		
		/// This constructor accepts a reflected struct and turns it into a resource data.
		/// The types are tried to be matched with the built-in data resources. Second parameter
		/// allows named reflection.
		template<class T_, class R_ = typename T_::ReflectionType>
		explicit Data(const T_ &values, const R_ &reflectionobj = T_::Reflection()) {
			Append(values, reflectionobj);
		}
		
		/// This constructor accepts a reflected struct and turns it into a resource data.
		/// The types are tried to be matched with the built-in data resources. Multiple objects
		/// having same member names can be saved to a single data using second parameter, which
		/// appends a prefix to object members. Third parameter allows named reflection. 
		template<class T_, class R_ = typename T_::ReflectionType>
		Data(const T_ &values, const std::string &prefix, const R_ &reflectionobj = T_::Reflection()) {
			Append(values, prefix, reflectionobj);
		}
		
		virtual GID::Type GetGID() const override { 
			return GID::Data;
		}
		
		/// This function accepts a reflected struct and appends it to the resource data.
		/// The types are tried to be matched with the built-in data resources. Multiple objects
		/// having same member names can be saved to a single data using second parameter, which
		/// appends a prefix to object members. Third parameter allows named reflection. 
		template<class T_, class R_ = typename T_::ReflectionType>
		typename std::enable_if<R_::IsGorgonReflection, void>::type 
		Append(const T_ &values, const R_ &reflectionobj = T_::Reflection()) {
			static_assert(R_::IsGorgonReflection, "The template argument R_ for this constructor should be reflection type of the struct.");
			
			Append(values, "", reflectionobj);
		}
		
		/// This function accepts a reflected struct and appends it to the resource data.
		/// The types are tried to be matched with the built-in data resources. Multiple objects
		/// having same member names can be saved to a single data using second parameter, which
		/// appends a prefix to object members. Third parameter allows named reflection. 
		template<class T_, class R_ = typename T_::ReflectionType>
		typename std::enable_if<R_::IsGorgonReflection, void>::type  
		Append(const T_ &values, const std::string &prefix, const R_ &reflectionobj = T_::Reflection()) {
			static_assert(R_::IsGorgonReflection, "The template argument R_ for this constructor should be reflection type of the struct.");

			append(values, prefix, reflectionobj, typename TMP::Generate<R_::MemberCount>::Type());
		}

		/// Appends the given data to the end of this data resource
		template<class T_>
		typename std::enable_if<!std::is_base_of<Base, T_>::value && !T_::ReflectionType::IsGorgonReflection, void>::type
		Append(T_ value) {
			Insert(value, items.GetCount());
		}

		/// Appends the given resource object to the end of this data resource
		template<class T_>
		typename std::enable_if<std::is_base_of<Base, T_>::value, void>::type
		Append(T_ &value) {
			Insert(value, items.GetCount());
		}

		/// Appends the given data to the end of this data resource with the specified name
		template<class T_>
		typename std::enable_if<!std::is_base_of<Base, T_>::value, void>::type 
		Append(const std::string &name, T_ value) {
			Insert(name, value, items.GetCount());
		}

		/// Appends the given resource object to the end of this data resource with the specified name
		template<class T_>
		typename std::enable_if<std::is_base_of<Base, T_>::value, void>::type
		Append(const std::string &name, T_ &value) {
			Insert(name, value, items.GetCount());
		}

		/// Inserts a data item to the given position
		void Insert(int value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}
		
		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, int value, int before) {
			items.Insert(new IntegerData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(float value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, float value, int before) {
			items.Insert(new FloatData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(const std::string &value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, const std::string &value, int before) {
			items.Insert(new TextData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(const char *value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, const char *value, int before) {
			items.Insert(new TextData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Point value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Geometry::Point value, int before) {
			items.Insert(new PointData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Pointf value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Geometry::Pointf value, int before) {
			items.Insert(new PointfData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Size value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Geometry::Size value, int before) {
			items.Insert(new SizeData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Rectangle value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Geometry::Rectangle value, int before) {
			items.Insert(new RectangleData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Bounds value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Geometry::Bounds value, int before) {
			items.Insert(new BoundsData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Geometry::Margin value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		void Insert(const std::string &name, Geometry::Margin value, int before) {
			items.Insert(new MarginData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Base *value, int before) {
			Insert("#"+String::From(items.GetCount()), value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Base *value, int before) {
			items.Insert(new ObjectData(name, value), before);
		}

		/// Inserts a data item to the given position
		void Insert(Base &value, int before) {
			Insert(&value, before);
		}

		/// Inserts a data item to the given position with the specified name
		void Insert(const std::string &name, Base &value, int before) {
			Insert(name, &value, before);
		}

		/// Transforms the members of this resource data to the given struct. Members are matched by name. This version
		/// does not transfer the ownership of the object data.
		template<class T_, class R_ = typename T_::ReflectionType>
		T_ NamedTransform_KeepObjects(const R_ &reflectionobj = T_::Reflection()) const {
			return NamedTransform_KeepObjects<T_, R_>("", reflectionobj);
		}
		
		/// Transforms the members of this resource data to the given struct. Members are matched by name starting
		/// with prefix. This version does not transfer the ownership of the object data.
		template<class T_, class R_ = typename T_::ReflectionType>
		T_ NamedTransform_KeepObjects(const std::string &prefix, const R_ &reflectionobj = T_::Reflection()) const {
			T_ t;
			namedtransform<T_, R_>(t, prefix, reflectionobj, typename TMP::Generate<R_::MemberCount>::Type());
			
			return t;
		}

		/// Transforms the members of this resource data to the given struct. Members are matched by name. Any
		/// resource objects that are transformed into the structure are released.
		template<class T_, class R_ = typename T_::ReflectionType>
		T_ NamedTransform(const R_ &reflectionobj = T_::Reflection()) {
			return NamedTransform<T_, R_>("", reflectionobj);
		}
		
		/// Transforms the members of this resource data to the given struct. Members are matched by name starting
		/// with prefix. Any resource objects that are transformed into the structure are released.
		template<class T_, class R_ = typename T_::ReflectionType>
		T_ NamedTransform(const std::string &prefix, const R_ &reflectionobj = T_::Reflection()) {
			T_ t;
			namedtransform<T_, R_>(t, prefix, reflectionobj, typename TMP::Generate<R_::MemberCount>::Type());
			
			return t;
		}
		
		/// Returns the data with the given index; use GetObject in order to get resource objects.
		template<class T_>
		T_ Get(int index) const {
			static_assert(std::is_same<T_, int>::value, "Unknown data type.");
		}
		
		/// Returns the data with the given name; use GetObject in order to get resource objects.
		template<class T_>
		T_ Get(const std::string &name) const {
			static_assert(std::is_same<T_, int>::value, "Unknown data type.");
		}

		/// Returns the resource object at the given index
		template<class T_>
		T_ &GetObject(int index) const {
			auto &item=dynamic_cast<ObjectData&>(items[index]);

			return item.Get<T_>();
		}

		/// Returns the resource object with the given name
		template<class T_>
		T_ &GetObject(const std::string &name) const {
			Utils::NotImplemented();
		}

		/// Can be used to iterate over data objects
		ConstIterator begin() const {
			return items.begin();
		}

		/// Can be used to iterate over data objects
		ConstIterator end() const {
			return items.end();
		}

		/// Can be used to iterate over data objects
		Iterator begin() {
			return items.begin();
		}

		/// Can be used to iterate over data objects
		Iterator end() {
			return items.end();
		}

		/// Returns the data item with the given index
		DataItem &GetItem(int index) const {
			return items[index];
		}
		
		/// Returns the data item with the given name
		DataItem &GetItem(const std::string &name) const {
			for(auto &item : items) {
				if(item.Name==name) return item;
			}

			throw std::runtime_error("Cannot find the item requested: "+name);
		}
		
		/// Returns the index of the data item with the given name. This function will return -1 
		/// if there is no such data item with the specified name exists
		int FindIndex(const std::string &name) const {
			int ind=0;
			for(auto &item : items) {
				if(item.Name==name) return ind;
				ind++;
			}

			return -1;
		}
		
		/// Returns the number data items in this data resource
		int GetCount() const {
			return items.GetCount();
		}
		
		/// Removes the item at the given index. The data item will be destroyed.
		void Remove(int index) {
			items.Delete(index);
		}

		/// Removes the item with the given name. The data item will be destroyed.
		void Remove(const std::string &name) {
			int ind = FindIndex(name);

			if(ind==-1) {
				throw std::runtime_error("Cannot find the item with the name: "+name);
			}

			Remove(ind);
		}

		/// Releases the data item with the given index. The data item will not be destroyed
		DataItem &Release(int index) {
			auto &item = items[index];

			items.Remove(index);

			return item;
		}

		/// Releases the data item with the given name. The data item will not be destroyed
		DataItem &Release(const std::string &name) {
			int ind = FindIndex(name);

			if(ind==-1) {
				throw std::runtime_error("Cannot find the item with the name: "+name);
			}

			return Release(ind);
		}

		/// Adds the given data item to this data resource. Ownership of the item is transferred to this Data
		void Add(DataItem &item) {
			items.Add(item);
		}
		
		/// Inserts the given data item to this data resource before the specified index. Ownership of the item is transferred to this Data
		void Insert(DataItem &item, int before) {
			items.Insert(item, before);
		}
		
		/// Loads a data resource
		static Data *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);
		
	protected:
		/// Destructor
		~Data() {
			items.Destroy();
		}

	private:
		virtual void save(Writer &writer) const override;
		
		Containers::Collection<DataItem> items;
		
		
	};

	template<>
	inline int Data::Get<int>(int index) const {
		auto &item=dynamic_cast<IntegerData&>(items[index]);

		return item.Get();
	}

	template<>
	inline float Data::Get<float>(int index) const {
		auto &item=dynamic_cast<FloatData&>(items[index]);

		return item.Get();
	}

	template<>
	inline std::string Data::Get<std::string>(int index) const {
		auto &item=dynamic_cast<TextData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Point Data::Get<Geometry::Point>(int index) const {
		auto &item=dynamic_cast<PointData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Pointf Data::Get<Geometry::Pointf>(int index) const {
		auto &item=dynamic_cast<PointfData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Size Data::Get<Geometry::Size>(int index) const {
		auto &item=dynamic_cast<SizeData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Rectangle Data::Get<Geometry::Rectangle>(int index) const {
		auto &item=dynamic_cast<RectangleData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Bounds Data::Get<Geometry::Bounds>(int index) const {
		auto &item=dynamic_cast<BoundsData&>(items[index]);

		return item.Get();
	}

	template<>
	inline Geometry::Margin Data::Get<Geometry::Margin>(int index) const {
		auto &item=dynamic_cast<MarginData&>(items[index]);

		return item.Get();
	}


}
