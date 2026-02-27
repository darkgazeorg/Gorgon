/// @file OS.h contains operating system functionality. This
///       file does not include any operating system headers       

#pragma once

#include <string>
#include <vector>
#include "Enum.h"

namespace Gorgon {
	/// This namespace contains operating system related functionality.
	/// All functions here behaves same way in all supported operating
	/// systems.
	namespace OS {

		/// Initializes operating system module.
		void Initialize();

		/// Contains user related information and services
		namespace User {
			/// Returns the current username
			std::string GetUsername();

			/// Returns the name of the current user
			std::string GetName();

			/// Returns the path where documents of the user should be saved
			std::string GetDocumentsPath();

			/// Returns the home directory of the user
			std::string GetHomePath();

			/// Returns the path where applications can save data related to this user. This path
			/// could simply be user's home directory. Best practice would be creating a directory
			/// that starts with a "." such as .gorgon
			std::string GetDataPath();
			
			
			/// Check if the currently logged in user is an administrator. If you are looking to perform
			/// an elevated operation, you best try to perform the operation without checking if the user
			/// is an admin first. It might be possible for a regular user to have extra permissions.
			/// Check this function afterwards before asking for elevation.
			bool IsAdmin();
		}

		/// Opens a terminal window to display output from the stdout. This is not required
		/// for most operating systems and will not perform anything unless its necessary.
		void OpenTerminal();

		/// This function shows a OS message box to display errors, for other messages
		/// its better to use in-game dialogs
		void DisplayMessage(const std::string &message);

		/// Returns the directory where the system wide application data is stored. Most probably
		/// it will be read only
		std::string GetAppDataPath();

		/// Returns the directory where the system wide application settings is stored. Most probably
		/// it will be read only
		std::string GetAppSettingPath();
		
		/// Returns the value of an environment variable.
		std::string GetEnvVar(const std::string &var);

		/// Returns the name of the current operating system in human readable form. Might change from
		/// installation to installation.
		std::string GetName();

		/// This structure represents the version of the operating system
		struct Info {
			enum OSType {
				Windows,
				Linux
			};

			/// Identifier for the current operating system
			OSType Type;
			
			/// Name of the current operating system
			std::string Name;

			/// Major version
			int Major;
			
			/// Minor version
			int Minor;
			
			/// Revision
			int Revision;
			
			/// Build number
			int BuildNumber;
			
			/// Number of bits in the architecture, this number is independent of compiled platform
			/// of Gorgon Library
			int ArchBits;
			
			/// The name  of the architecture, this information is independent of compiled platform
			/// of Gorgon Library
			std::string Arch;
		};
		
		/// Returns information related with the operating system, including version, name, and architecture.
		Info GetInfo();
		
		/// Starts the given application. This application is searched from the installed applications
		/// unless it includes a path. You may use `./appname` to start the appname from the current
		/// directory. The application is started in a separate process and the current process does not
		/// stop to wait its execution.
		/// @warning Depending on the operating system, executables in the current directory might take
		///          precedence even if no path is given.
		/// @return  true if the program is found and started. Does not check if the program continues
		///          working properly
		bool Start(const std::string &name, const std::vector<std::string> &args=std::vector<std::string>());
		
		/// Opens the given file with the related application. This can also be a URI.
		/// @return true if the given file is somehow opened. This includes open with dialog if it can
		///         be displayed
		bool Open(const std::string &file);

		/// This method will notify the system should process any messages that coming from the operating
		/// system. Internally used. Should only be used when necessary.
		void processmessages();
        
        enum class FontWeight {
            Thin        = 100,
            ExtraLight  = 200,
            Light       = 300,
            Regular     = 400,
            Medium      = 500,
            SemiBold    = 600,
            Bold        = 700,
            ExtraBold   = 800,
            Heavy       = 900,
        };
        
        DefineEnumStrings(FontWeight, {
            {FontWeight::Thin, "Thin"},
            {FontWeight::ExtraLight, "Extra light"},
            {FontWeight::ExtraLight, "ExtraLight"},
            {FontWeight::Light, "Light"},
            {FontWeight::Regular, "Regular"},
            {FontWeight::Medium, "Medium"},
            {FontWeight::SemiBold, "Semi bold"},
            {FontWeight::SemiBold, "SemiBold"},
            {FontWeight::Bold, "Bold"},
            {FontWeight::ExtraBold, "Extra bold"},
            {FontWeight::ExtraBold, "ExtraBold"},
            {FontWeight::Heavy, "Heavy"},
        });
        
        /// This class represents a single font. 
        class Font {
        public:
            
            /// Returns the named weight of the font.
            FontWeight GetWeight() const;
            
            /// Returns CSS usable weight of the font.
            std::string GetCSSWeight() const {
                if(Weight == 400)
                    return "normal";
                else if(Weight == 600)
                    return "bold";
                else
                    return std::to_string(Weight);
            }
            
            /// This is the style name of the font. Apart form Regular, Bold, Italic
            /// it could have different names such as narrow, Extra-bold
            std::string Style = "Regular";
            
            /// If this is a bold font
            bool Bold = false;
            
            /// Weight of the font. This is same as CSS3, 400 is regular
            int  Weight = (int)FontWeight::Regular;
            
            /// Width of the font, 100 is regular, 75 is condensed and 125 is expanded.
            int Width   = 100;
            
            /// Whether the font is italic or not
            bool Italic = false;
            
            /// Whether the font is monospaced
            bool Monospaced = false;
            
            /// Name of the font family
            std::string Family = "";
            
            /// Filename of the font
            std::string Filename = "";
        };
        
        /**
         * This structure stores information about a font family installed in the system. Once a
         * family is obtained, its type faces can be queried. In some systems, all faces will be
         * loaded along with the families
         */
        class FontFamily {
        public:
            /// Name of the font family
            std::string Family;
            
            /// Individual fonts in the family
            std::vector<Font> Faces;
        };
        
        /// Returns a list of font families installed in the system. Depending on the operating system,
        /// this function might be slow as it might need to load each font file to obtain information.
        std::vector<FontFamily> GetFontFamilies();

        /// Returns a font with the given family and style name. Supports type names such as
        /// "serif", "sans-serif", "monospace" and "cursive" as family names. Style name can be "Regular",
        /// "Bold", "Italic" or "Bold Italic" or any other style name that the font file contains. 
        /// If the font is not found a fallback font will be returned. This function is not guaranteed to 
        /// return the same font for the same family and style name across different calls. Check second
        /// value of the returned pair to check if the returned font is a fallback or not.
        std::pair<Font, bool /*exact match*/> GetFont(const std::string &familyname = "", const std::string &stylename = "");
        
        void DumpFontFamilies(std::ostream &out);
	}
}
