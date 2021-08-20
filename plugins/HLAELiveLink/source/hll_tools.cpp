// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_tools.h"
#include "maxon/logger.h"
#include "maxon/string.h"
#include <functional>
#include "c4d_symbols.h"

namespace HLL::Tools
{
	class XmlUnorderedMap
	{
	public:
		XmlUnorderedMap() = default;
		XmlUnorderedMap(maxon::IoXmlNodeInterface * xmlHead)
		{
			std::function<XmlUnorderedMap(maxon::IoXmlNodeInterface*)> func = [&func](maxon::IoXmlNodeInterface* node) -> XmlUnorderedMap
			{
				XmlUnorderedMap rval;

				auto it = node;
				maxon::String key = it->GetName();

				auto bIt = it;
				while (it->GetUp() != nullptr)
				{
					it = it->GetUp();
					key = it->GetName() + "." + key;
				}
				it = bIt;

				rval.Emplace(key, it);

				if (it->GetNext() || it->GetDown())
				{
					if (it->GetDown()) rval.Emplace(func(it->GetDown()));
					if (it->GetNext()) rval.Emplace(func(it->GetNext()));
				}

				return rval;
			};

			*this = func(xmlHead);
		}

		void Emplace(maxon::String key, maxon::IoXmlNodeInterface* val)
		{
			_keys.emplace_back(key);
			_values.emplace_back(val);
		}

		void Emplace(XmlUnorderedMap map)
		{
			for (size_t i = 0; i < map._keys.size(); i++)
			{
				this->_keys.emplace_back(map._keys[i]);
				this->_values.emplace_back(map._values[i]);
			}
		}

		maxon::IoXmlNodeInterface* operator[] (const maxon::String &key)
		{
			for (size_t i = 0; i < this->_keys.size(); i++)
				if (this->_keys[i] == key)
					return this->_values[i];
			return maxon::IoXmlNodeInterface::Alloc(maxon::SourceLocation());
		}

		const maxon::IoXmlNodeInterface* operator[] (const maxon::String &key) const
		{
			for (size_t i = 0; i < this->_keys.size(); i++)
				if (this->_keys[i] == key)
					return this->_values[i];
			return maxon::IoXmlNodeInterface::Alloc(maxon::SourceLocation());
		}

	private:
		std::vector<maxon::String> _keys;
		std::vector<maxon::IoXmlNodeInterface*> _values;
	};

	Bool CheckForUpdates(String * link, const String & url)
	{
		maxon::Url u(url);
		Int e;
		auto res = maxon::IoXmlParser::ReadDocument(u, e);

		if (res == maxon::FAILED)
		{
			auto er = res.GetError();
			*link = er.GetMessage();
			return false;
		}

		auto xml_map = XmlUnorderedMap(res.GetValue());
		Int32 maj = xml_map["update.version.major"_s]->GetValue().ToInt32().GetValue();
		Int32 min = xml_map["update.version.minor"_s]->GetValue().ToInt32().GetValue();
		*link = xml_map["update.link"_s]->GetValue();

		if (maj > Globals::Version::major) return true;
		if (maj == Globals::Version::major && min > Globals::Version::minor) return true;

		return false;
	}

	static const maxon::Id loggerid("com.thatnwp.xNWP.livelinklogger");
	class LoggerImpl;
	static LoggerImpl* g_loggerInstance;

	class LoggerImpl
	{
		maxon::LoggerRef _logger;

	public:
		static void Write(const maxon::String msg)
		{
			if (g_loggerInstance == nullptr)
			{
				g_loggerInstance = new LoggerImpl;
				g_loggerInstance->_logger = maxon::LoggerRef::Create().UncheckedGetValue();
				g_loggerInstance->_logger.AddLoggerType(maxon::TARGETAUDIENCE::ALL, maxon::LoggerTypes::Application()).UncheckedGetValue();
				g_loggerInstance->_logger.SetName("HLAELiveLink"_s);
				maxon::Loggers::Insert(loggerid, g_loggerInstance->_logger).UncheckedGetValue();
				const maxon::Int32 BFM_REBUILDCONSOLETREE = 334295845;
				SpecialEventAdd(BFM_REBUILDCONSOLETREE);
			}

			g_loggerInstance->_logger.Write(maxon::TARGETAUDIENCE::ALL, msg, maxon::SourceLocation::NullValue()).UncheckedGetValue();
		}
	};

	void Log(String msg)
	{
		LoggerImpl::Write(msg);
	}

	void LogError(String msg)
	{
		LoggerImpl::Write("ERROR: " + msg);
	}

	UserConfig::UserConfig(const Filename& file)
	{
		LoadFile(file);
	}

	void UserConfig::SaveFile(const Filename& file)
	{
		Filename userFile(GeGetPluginPath() + "\\" + file);

		auto bToStr = [](Bool b) -> maxon::String { return b ? "true"_s : "false"_s; };
		auto iToStr = [](Int32 i) -> maxon::String { return String::IntToString(i); };
		auto fToStr = [](Float32 f)->maxon::String {return maxon::String::FloatToString(Floor(RadToDeg(f) + 0.005), -1, 2); };

		// Create head
		auto XmlInterface = maxon::IoXmlNodeInterface::Alloc(maxon::SourceLocation());
		XmlInterface->SetName("settings"_s);

		// User Settings
		auto node_user = XmlInterface->AddChild("user"_s);

		auto node = node_user->AddChild("checkforupdates"_s); node->SetValue(iToStr(checkforupdates));	// Check For Updates
		node = node_user->AddChild("hostname"_s); node->SetValue(hostname);								// Hostname
		node = node_user->AddChild("port"_s); node->SetValue(iToStr(port));								// Port
		node = node_user->AddChild("pollrate"_s); node->SetValue(iToStr(pollrate));						// Pollrate
		node = node_user->AddChild("globalcoords"_s); node->SetValue(bToStr(globalcoords));				// Global Coordinates

		auto node_orientation = node_user->AddChild("orientation"_s);
		node = node_orientation->AddChild("x"_s); node->SetValue(fToStr(orientation.x));				// Orientation X
		node = node_orientation->AddChild("y"_s); node->SetValue(fToStr(orientation.y));				// Orientation Y
		node = node_orientation->AddChild("z"_s); node->SetValue(fToStr(orientation.z));				// Orientation Z

		// Write file
		maxon::Url u("file:///" + userFile.GetString());
		maxon::IoXmlParser::WriteDocument(u, XmlInterface, true, nullptr).GetValue();
	}

	Bool UserConfig::LoadFile(const Filename& file)
	{
		Filename userFile(GeGetPluginPath() + "\\" + file);
		maxon::Url u("file:///" + userFile.GetString());
		Int er;
		auto xResult = maxon::IoXmlParser::ReadDocument(u, er);

		if (xResult == maxon::FAILED)
		{
			auto err = xResult.GetError();
			String msg = GeLoadString(STR_USER_SETTINGS_ERROR);
			msg += " -- ";
			msg += err.ToString(nullptr);
			Tools::LogError(msg);
			return false;
		}

		auto xml_map = XmlUnorderedMap(xResult.GetValue());

		auto strToI = [](maxon::String str) -> Int32 { return str.ToInt32().GetValue(); };
		auto strToB = [](maxon::String str) -> Bool { return str == "true" ? true : false; };
		auto strToF = [](maxon::String str) -> Float32 { return DegToRad(str.ToFloat32().GetValue()); };

		checkforupdates = strToI(xml_map["settings.user.checkforupdates"_s]->GetValue());	// Check For Updates
		hostname = xml_map["settings.user.hostname"_s]->GetValue();							// Hostname
		port = strToI(xml_map["settings.user.port"_s]->GetValue());							// Port
		pollrate = strToI(xml_map["settings.user.pollrate"_s]->GetValue());					// Pollrate
		globalcoords = strToB(xml_map["settings.user.globalcoords"_s]->GetValue());			// Global Coordinates
		orientation.x = strToF(xml_map["settings.user.orientation.x"_s]->GetValue());		// Orientation X
		orientation.y = strToF(xml_map["settings.user.orientation.y"_s]->GetValue());		// Orientation Y
		orientation.z = strToF(xml_map["settings.user.orientation.z"_s]->GetValue());		// Orientation Z

		return true;
	}
}