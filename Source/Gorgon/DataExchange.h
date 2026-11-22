#pragma once

#include "Gorgon/String.h"
#include "Resource/GID.h"

#include <string>
#include <vector>

namespace Gorgon {


	/**
	* Base object for data to be exchanged. This object should be type casted to derived type to access
	* stored data. Name could be used to show the data type.
	*/
	class ExchangeData { // this class will be moved elsewhere
	public:
		virtual ~ExchangeData() {}

		/// Should return the name of the data type
		virtual std::string         Name() const = 0;

		/// Should return the type id of the data
		virtual Resource::GID::Type Type() const = 0;

		/// Should return the textual representation of the data. A datatype can simply return its typename
		virtual std::string			Text() const {
			return Name();
		}
	};

	/**
	* Stores text data for data exchange
	*/
	class TextData : public ExchangeData {
	public:
		explicit TextData(std::string text = "") : text(text) {}

		/// Changes the text in this data
		void SetText(std::string value) {
			text = value;
		}

		/// Returns the text in this data
		std::string GetText() const {
			return text;
		}

		virtual std::string Name() const override {
			return "Text";
		}

		virtual std::string Text() const override {
			return text;
		}

		virtual Resource::GID::Type Type() const override {
			return Resource::GID::Text;
		}

	private:
		std::string text;
	};

	/**
	* Stores list of files for data exchange
	*/
	class FileData : public ExchangeData {
	public:
		enum ExchangeAction {
			Move,
			Copy
		};

		explicit FileData(std::vector<std::string> files ={}) : files(std::move(files)) {}

		explicit FileData(std::string file) : files({file}) {}

		/// Adds a new file to the list
		void AddFile(std::string value) {
			files.push_back(value);
		}

		/// Removes a file from the list
		void RemoveFile(int ind) {
			files.erase(files.begin()+ind);
		}

		/// Clears the file list
		void Clear() {
			files.clear();
		}

		/// Returns the number of files in the list
		int GetSize() const {
			return (int)files.size();
		}

		/// Returns the file at the given index
		std::string operator[] (int ind) const {
			return files[ind];
		}

		/// To allow ranged based iteration
		std::vector<std::string>::const_iterator begin() const {
			return files.begin();
		}

		/// To allow ranged based iteration
		std::vector<std::string>::const_iterator end() const {
			return files.end();
		}

		/// To allow ranged based iteration
		std::vector<std::string>::iterator begin() {
			return files.begin();
		}

		/// To allow ranged based iteration
		std::vector<std::string>::iterator end() {
			return files.end();
		}

		virtual std::string Name() const override {
			return "File";
		}

		virtual Resource::GID::Type Type() const override {
			return Resource::GID::File;
		}

		virtual std::string Text() const override {
			return String::Join(files, "; ");
		}

		ExchangeAction Action = Copy;

	private:
		std::vector<std::string> files;
	};

}