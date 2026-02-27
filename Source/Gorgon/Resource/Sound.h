#pragma once

#include <vector>
#include <memory>

#include "../Types.h"
#include "Base.h"
#include "../Containers/Wave.h"

namespace Gorgon :: Resource {
		class File;
		class Reader;

		/// This is sound resource. Supports 
		class Sound : public Base {
		public:

			/// Default constructor
			Sound() {}

			/// Destructor
			virtual ~Sound() {}

			/// 04010000h (Extended, Sound)
			virtual GID::Type GetGID() const override { return GID::Sound; }

			const Containers::Wave &GetWave() const {
				return data;
			}

			Containers::Wave &GetWave() {
				return data;
			}

			/// Destroys the data stored in the sound
			void Destroy() {
				isloaded=false;
			}

			/// Loads the sound from the disk. If sound is already loaded, this function will return true
			bool Load();

			/// Returns whether the sound data is loaded
			bool IsLoaded() const { return isloaded; }

			/// This function loads a sound resource from the given file
			static Sound *LoadResource(std::weak_ptr<File> file, std::shared_ptr<Reader> reader, unsigned long size);

			/// Assigns the sound to the copy of the given data. Ownership of the given data
			/// is not transferred. If the given data is not required elsewhere, consider using
			/// Assume function. Also sets IsLoaded.
			void Assign(const Containers::Wave &wave) {
				data=wave.Duplicate();

				isloaded = true;
			}

			/// Assumes the contents of the given wave as wave data. The given parameter is moved from
			/// and will become empty. Also sets IsLoaded.
			void Assume(Containers::Wave &wave) {
				data = std::move(wave);

				isloaded = true;
			}

			int GetBits() const {
				return bits;
			}

			/// Sets the number of bits per sample. Only effects saving, in memory format
			/// is non-PCM 32bit float. Currently this does not have effect if data is saved
			/// non-PCM fashion.
			void SetBits(int bits) {
				this->bits = bits;

				checkfmt();
			}

			/// Returns if the wave data will be saved as PCM data.
			bool IsPCM() const {
				return pcm;
			}

			/// Set whether the wave data should be saved in PCM format. FLAC supports only 
			/// PCM while internal saving mechanism can save wave non-PCM mode as 32bit float.
			void SetPCM(bool pcm) {
				this->pcm = pcm;

				checkfmt();
			}

			/// Returns the compression type of this resource.
			GID::Type GetCompression() const {
				return compression;
			}

			/// Changes the compression type of this resource. Currently GID::None and 
			/// GID::FLAC is supported. If Flac support is disabled, GID::FLAC will cause
			/// a runtime_error during save.
			void SetCompression(GID::Type compression) {
				this->compression = compression;
			
				checkfmt();
			}


		protected:

			/// Loads the sound from the data stream
			bool load(std::shared_ptr<Reader> reader, unsigned long size, bool forceload);

			void save(Writer &writer) const override;

			/// Checks if the format of the file is well-formed
			void checkfmt() const;

			/// Entry point of this resource within the physical file. This value is stored for 
			/// late loading purposes
			unsigned long entrypoint = -1;

			/// Used to handle late loading
			std::shared_ptr<Reader> reader;

			/// Whether this sound is loaded or not
			bool isloaded = false;

			/// Compression mode of this sound resource
			GID::Type compression = GID::FLAC;

			/// Whether to load this sound during initial loading
			bool lateloading = false;

			bool pcm = true;
			
			/// Sound data
			Containers::Wave data;

			/// Number of bits per sample
			int bits = 16;
		};
	}
