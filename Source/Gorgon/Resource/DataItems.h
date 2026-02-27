#pragma once

#include "Base.h"
#include "Reader.h"
#include "../Geometry/Point.h"
#include "../Geometry/Size.h"
#include "../Geometry/Rectangle.h"
#include "../Geometry/Bounds.h"
#include "../Geometry/Margin.h"

#include <functional>

namespace Gorgon :: Resource {

	class File;

	class DataItem {
	public:

		using LoaderFn=std::function<DataItem *(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize)>;

		virtual ~DataItem() {}

		/// Returns the Gorgon ID of this data object
		virtual GID::Type GetGID() const = 0;

		/// The name of the data item
		std::string Name;

		/// Saves the data item with header information to gorgon file
		virtual void Save(Writer &writer) const {
			auto start=writer.WriteChunkStart(GetGID());
			writer.WriteStringWithSize(Name);
			SaveValue(writer);
			writer.WriteEnd(start);
		}

		/// Returns the contents of this data item to the requested type; use GetObject in order to get resource objects
		template <class T_>
		T_ Get() const {
			static_assert(std::is_same<T_, int>::value, "Unknown data type.");
		}

		template <class T_>
		T_ &GetObject() const;

		/// Converts the contents of this data to string
		virtual std::string ToString() const = 0;

		/// Saves only the value of the data item to gorgon file. issizewritten controls whether the size of the object
		/// should be written for some data types (like blob and string)
		virtual void SaveValue(Writer &writer) const = 0;

		/// This function will initialize data loaders
		static void InitializeLoaders();

		/// Data loaders that all data containing classes would use to load their children
		static std::map<GID::Type, LoaderFn> DataLoaders;
	};

	namespace internal {
		template<class T_>
		class DataImp {
		public:
			DataImp() = default;

			DataImp(T_ v) : value(v) {}

			T_ Get() const { return value; }

			void Set(T_ val) { value = val; }

		protected:
			T_ value = T_();
		};
	}

	class IntegerData : public DataItem, private internal::DataImp<int> {
	public:
		IntegerData() {}

		IntegerData(int v) : DataImp<int>(v) {}

		IntegerData(const std::string &name, int v) : DataImp<int>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Int; }

		using internal::DataImp<int>::Get;
		using internal::DataImp<int>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class FloatData : public DataItem, private internal::DataImp<float> {
	public:
		FloatData() {}

		FloatData(float v) : DataImp<float>(v) {}

		FloatData(const std::string &name, float v) : DataImp<float>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Float; }

		using internal::DataImp<float>::Get;
		using internal::DataImp<float>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteFloat(value);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class TextData : public DataItem, private internal::DataImp<std::string> {
	public:
		TextData() {}

		TextData(std::string v) : DataImp<std::string>(v) {}

		TextData(const std::string &name, const std::string &v) : DataImp<std::string>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Text; }

		using internal::DataImp<std::string>::Get;
		using internal::DataImp<std::string>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteStringWithSize(value);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return value;
		}
	};

	class PointData : public DataItem, private internal::DataImp<Geometry::Point> {
	public:
		PointData() {}

		PointData(Geometry::Point v) : DataImp<Geometry::Point>(v) {}

		PointData(const std::string &name, Geometry::Point v) : DataImp<Geometry::Point>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Point; }

		using internal::DataImp<Geometry::Point>::Get;
		using internal::DataImp<Geometry::Point>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value.X);
			writer.WriteInt32(value.Y);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class PointfData : public DataItem, private internal::DataImp<Geometry::Pointf> {
	public:
		PointfData() {}

		PointfData(Geometry::Pointf v) : DataImp<Geometry::Pointf>(v) {}

		PointfData(const std::string &name, Geometry::Pointf v) : DataImp<Geometry::Pointf>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Pointf; }

		using internal::DataImp<Geometry::Pointf>::Get;
		using internal::DataImp<Geometry::Pointf>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteFloat(value.X);
			writer.WriteFloat(value.Y);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class SizeData : public DataItem, private internal::DataImp<Geometry::Size> {
	public:
		SizeData() {}

		SizeData(Geometry::Size v) : DataImp<Geometry::Size>(v) {}

		SizeData(const std::string &name, Geometry::Size v) : DataImp<Geometry::Size>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Size; }

		using internal::DataImp<Geometry::Size>::Get;
		using internal::DataImp<Geometry::Size>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value.Width);
			writer.WriteInt32(value.Height);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class RectangleData : public DataItem, private internal::DataImp<Geometry::Rectangle> {
	public:
		RectangleData() {}

		RectangleData(Geometry::Rectangle v) : DataImp<Geometry::Rectangle>(v) {}

		RectangleData(const std::string &name, Geometry::Rectangle v) : DataImp<Geometry::Rectangle>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Rectangle; }

		using internal::DataImp<Geometry::Rectangle>::Get;
		using internal::DataImp<Geometry::Rectangle>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value.X);
			writer.WriteInt32(value.Y);
			writer.WriteInt32(value.Width);
			writer.WriteInt32(value.Height);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class BoundsData : public DataItem, private internal::DataImp<Geometry::Bounds> {
	public:
		BoundsData() {}

		BoundsData(Geometry::Bounds v) : DataImp<Geometry::Bounds>(v) {}

		BoundsData(const std::string &name, Geometry::Bounds v) : DataImp<Geometry::Bounds>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Bounds; }

		using internal::DataImp<Geometry::Bounds>::Get;
		using internal::DataImp<Geometry::Bounds>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value.Left);
			writer.WriteInt32(value.Top);
			writer.WriteInt32(value.Right);
			writer.WriteInt32(value.Bottom);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class MarginData : public DataItem, private internal::DataImp<Geometry::Margin> {
	public:
		MarginData() {}

		MarginData(Geometry::Margin v) : DataImp<Geometry::Margin>(v) {}

		MarginData(const std::string &name, Geometry::Margin v) : DataImp<Geometry::Margin>(v) {
			Name=name;
		}

		virtual GID::Type GetGID() const override { return GID::Data_Margin; }

		using internal::DataImp<Geometry::Margin>::Get;
		using internal::DataImp<Geometry::Margin>::Set;

		virtual void SaveValue(Writer &writer) const override {
			writer.WriteInt32(value.Left);
			writer.WriteInt32(value.Top);
			writer.WriteInt32(value.Right);
			writer.WriteInt32(value.Bottom);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			return String::From(value);
		}
	};

	class ObjectData : public DataItem {
	public:
		ObjectData() {}

		ObjectData(Base &v) : value(&v) {}

		ObjectData(const std::string &name, Base &v) : value(&v) {
			Name=name;
		}

		ObjectData(Base *v) : value(v) {}

		ObjectData(const std::string &name, Base *v) : value(v) {
			Name=name;
		}

		virtual	~ObjectData() {
			if(value)
				value->DeleteResource();
		}

		virtual GID::Type GetGID() const override { return GID::Data_Object; }

		virtual void SaveValue(Writer &writer) const override {
			if(value) {
				value->Save(writer);
			}
		}

		/// Changes the object that this data holds. Once set, the data owns the object.
		void Set(Base &v) {
			if(value && value!=&v)
				value->DeleteResource();

			value=&v;
		}

		/// Changes the object that this data holds. Once set, the data owns the object.
		void Set(Base *v) {
			if(value && value!=v)
				value->DeleteResource();

			value=v;
		}

		/// Removes the object that this data holds
		void Unset() {
			if(value)
				value->DeleteResource();

			value=nullptr;
		}

		/// Removes the object from this data without destroying it.
		Base &Release() {
			auto temp=value;
			value=nullptr;

			return *temp;
		}
		
		bool IsSet() const {
			return value!=nullptr;
		}

		/// Returns the item contained within this object
		template<class T_>
		T_ &Get() const {
			if(!value)
				throw std::runtime_error("This data does not contain any object.");

			return dynamic_cast<T_ &>(*value);
		}

		static DataItem *Load(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long totalsize);

		virtual std::string ToString() const override {
			if(value)
				return value->GetName();
			else
				return "[Empty]";
		}

	private:
		Base *value = nullptr;
	};

	inline std::ostream &operator <<(std::ostream &out, const DataItem &item) {
		out<<item.ToString();

		return out;
	}


	template<>
	inline int DataItem::Get<int>() const {
		auto &item=dynamic_cast<const IntegerData&>(*this);

		return item.Get();
	}

	template<>
	inline float DataItem::Get<float>() const {
		auto &item=dynamic_cast<const FloatData&>(*this);

		return item.Get();
	}

	template<>
	inline std::string DataItem::Get<std::string>() const {
		auto &item=dynamic_cast<const TextData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Point DataItem::Get<Geometry::Point>() const {
		auto &item=dynamic_cast<const PointData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Pointf DataItem::Get<Geometry::Pointf>() const {
		auto &item=dynamic_cast<const PointfData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Size DataItem::Get<Geometry::Size>() const {
		auto &item=dynamic_cast<const SizeData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Rectangle DataItem::Get<Geometry::Rectangle>() const {
		auto &item=dynamic_cast<const RectangleData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Bounds DataItem::Get<Geometry::Bounds>() const {
		auto &item=dynamic_cast<const BoundsData&>(*this);

		return item.Get();
	}

	template<>
	inline Geometry::Margin DataItem::Get<Geometry::Margin>() const {
		auto &item=dynamic_cast<const MarginData&>(*this);

		return item.Get();
	}

	template <class T_>
	T_ &DataItem::GetObject() const {
		auto &item=dynamic_cast<const ObjectData&>(*this);

		return item.Get<T_>();
	}
}
