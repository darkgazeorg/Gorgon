#include "../Filesystem.h"
#include "../Encoding/LZMA.h"

#include "File.h"
#include "Blob.h"
#include "Image.h"
#include "Animation.h"
#include "Data.h"
#include "Sound.h"
#include "Font.h"
#include "Pointer.h"
#include "Line.h"
#include "Rectangle.h"
#include "MaskedObject.h"
#include "TintedObject.h"
#include "ScalableObject.h"
#include "StackedObject.h"



namespace Gorgon :: Resource {

	File::File() : root(new Folder), self(this, [](File*) {}) {
		Loaders[GID::Folder] 			= {GID::Folder			, Folder::LoadResource};
		Loaders[GID::Blob] 				= {GID::Blob			, Blob::LoadResource};
		Loaders[GID::Image]  			= {GID::Image			, Image::LoadResource};
		Loaders[GID::Animation_Image] 	= {GID::Animation_Image	, Image::LoadResource};
		Loaders[GID::Animation] 		= {GID::Animation		, Animation::LoadResource};
		Loaders[GID::Data] 				= {GID::Data			, Data::LoadResource};
		Loaders[GID::Sound]				= {GID::Sound  	        , Sound::LoadResource};
		Loaders[GID::Font]				= {GID::Font  	        , Font::LoadResource};
		Loaders[GID::Pointer]			= {GID::Pointer	        , Pointer::LoadResource};
		Loaders[GID::Line]				= {GID::Line	        , Line::LoadResource};
		Loaders[GID::Rectangle]			= {GID::Rectangle       , Rectangle::LoadResource};
		Loaders[GID::MaskedObject]		= {GID::MaskedObject    , MaskedObject::LoadResource};
		Loaders[GID::TintedObject]		= {GID::TintedObject    , TintedObject::LoadResource};
		Loaders[GID::ScalableObject]	= {GID::ScalableObject  , ScalableObject::LoadResource};
		Loaders[GID::StackedObject]		= {GID::StackedObject   , StackedObject::LoadResource};
	}

	bool Resource::Reader::ReadCommonChunk(Base &self, GID::Type gid, unsigned long size) {
		if(gid==GID::SGuid) {
			self.SetGuid(ReadGuid());
			return true;
		}
		else if(gid==GID::Name) {
			self.SetName(ReadString(size));
			return true;
		}

		return false;
	}

	Base *File::LoadChunk(Base &self, GID::Type gid, unsigned long size, bool skipobjects) {
		ASSERT(reader, "Reader is not open");

		if(reader->ReadCommonChunk(self, gid, size) || skipobjects) {
			return nullptr;
		}
		
		if(gid == GID::Null)
            return nullptr;

		for(auto &loader : Loaders) {

			if(loader.second.GID==gid) {
				auto obj=loader.second.Handler(this->self, reader, size);
#ifndef NDEBUG
				if(!obj) throw std::runtime_error("Cannot load the object with GID"+String::From(gid));
#endif
				return obj;
			}
		}

#ifndef NDEBUG
		throw std::runtime_error("No handler for GID"+String::From(gid));
#endif

		reader->EatChunk(size);

		return nullptr;
	}

	Base *File::LoadChunk(GID::Type gid, unsigned long size) {
		ASSERT(reader, "Reader is not open");
		
		if(gid == GID::Null)
            return nullptr;

		for(auto &loader : Loaders) {

			if(loader.second.GID==gid) {
				auto obj=loader.second.Handler(this->self, reader, size);
#ifndef NDEBUG
				if(!obj) throw std::runtime_error("Cannot load the object with GID"+String::From(gid));
#endif
				return obj;
			}
		}

#ifndef NDEBUG
		throw std::runtime_error("No handler for GID"+String::From(gid));
#endif

		reader->EatChunk(size);

		return nullptr;
	}

	void File::load(bool first, bool shallow) {
		reader->Open();
		if(!reader->IsGood()) {
			throw LoadError(LoadError::FileCannotBeOpened);
		}

		reader->KeepOpen();

		try {
			char sgn[7];

			//Check file signature
			reader->ReadArray(sgn, 6);
			sgn[6]=0;
			if(std::string("GORGON")!=sgn)
				throw LoadError(LoadError::Signature);

			//Check file version
			fileversion=reader->ReadInt32();
			if(fileversion>CurrentVersion)
				throw LoadError(LoadError::VersionMismatch);

			//Load file type
			filetype=reader->ReadGID();

			//Check first element
			if(reader->ReadGID()!=GID::Folder)
				throw LoadError(LoadError::Containment);

			unsigned long size=reader->ReadUInt32();

			//Load first element
			delete root;
			root=new Folder(*this);
			root->load(reader, size, first, shallow, true);

			if(!root) {
				root=new Folder(*this);
				throw LoadError(LoadError::Containment);
			}
		}
		catch(...) {
			delete root;
			root=new Folder;

			reader.reset();

			throw;
		}

		reader->NoLongerNeeded();

		//build mapping
		std::vector<Containers::Collection<Base>::ConstIterator> openlist;
		openlist.push_back(root->begin());
		mapping[root->GetGuid()]=root;

		while(openlist.size()) {
			if(!openlist.back().IsValid()) {
				openlist.pop_back();
				continue;
			}

			auto &obj=(*openlist.back());
			openlist.back().Next();
			if(obj.Children.GetCount()>0)
				openlist.push_back(obj.begin());

			mapping[obj.GetGuid()]=&obj;
		}

		root->Resolve(*this);
	}

	void File::createfilereader(std::string filename) {
		reader.reset();

		if(!Filesystem::IsFile(filename) && Filesystem::IsFile(filename+".lzma")) {
			std::ifstream ifile(filename+".lzma", std::ios::binary);
			std::ofstream ofile(filename, std::ios::binary);
			Encoding::Lzma.Decode(ifile, ofile);
			Filesystem::Delete(filename+".lzma");
		}

		reader.reset(new FileReader(filename));
	}
	
	void File::save() const {
		writer->open(true);
		
		writer->WriteString("GORGON");
		writer->WriteUInt32(0x00010000);
		writer->WriteUInt32(filetype.AsInteger());
		
		this->root->save(*writer);		
		
		
		writer->close();
	}
	
	/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
	Writer::Marker Writer::WriteObjectStart(const Base &base) {
		ASSERT(stream, "Writer is not opened.");
		ASSERT(IsGood(), "Writer is failed.");

		WriteGID(base.GetGID());
		auto pos=Tell();
		WriteChunkSize(-1);

		WriteGID(GID::SGuid);
		WriteChunkSize(0x08);
		WriteGuid(base.GetGuid());
		
		if(base.GetName()!="") {
			WriteGID(GID::Name);
			WriteStringWithSize(base.GetName());
		}

		return {pos};
	}
	
	
	/// Writes the start of an object. Should have a matching WriteEnd with the returned marker.
	/// This variant allows a replacement GID.
	Writer::Marker Writer::WriteObjectStart(const Base &base, GID::Type type) {
		ASSERT(stream, "Writer is not opened.");
		ASSERT(IsGood(), "Writer is failed.");

		WriteGID(type);
		auto pos=Tell();
		WriteChunkSize(-1);

		WriteGID(GID::SGuid);
		WriteChunkSize(0x08);
		WriteGuid(base.GetGuid());
		
		if(base.GetName()!="") {
			WriteGID(GID::Name);
			WriteStringWithSize(base.GetName());
		}
		
		return {pos};
	}
	
	
	const std::string LoadError::ErrorStrings[] ={
		"Unknown error", 
		"Cannot find the file specified", 
		"Signature mismatch",
		"Version mismatch", 
		"The supplied file is does not contain any data or its representation is invalid.",
		"An unknown node is encountered in the file.",
		"Cannot open the file specified.",
		"No file object is associated with this resource."
	};
	
	const std::string WriteError::ErrorStrings[] ={
		"Unknown error", 
		"Cannot create the file specified", 
		"This object has no data to save"
	};
	
}
