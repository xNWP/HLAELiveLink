// Copyright (c) 2018 Brett Anthony. All rights reserved.
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "hll_tools.h"

void HLL::Tools::XmlUnorderedMap::Emplace(maxon::String key, maxon::IoXmlNodeInterface* val)
{
	this->_keys.push_back(key);
	this->_values.push_back(val);
}

void HLL::Tools::XmlUnorderedMap::Emplace(XmlUnorderedMap map)
{
	for (size_t i = 0; i < map._keys.size(); i++)
	{
		this->_keys.push_back(map._keys[i]);
		this->_values.push_back(map._values[i]);
	}
}

maxon::IoXmlNodeInterface* HLL::Tools::XmlUnorderedMap::operator[](const maxon::String &key)
{
	for (size_t i = 0; i < this->_keys.size(); i++)
		if (this->_keys[i] == key)
			return this->_values[i];
	return maxon::IoXmlNodeInterface::Alloc(maxon::SourceLocation());
}

HLL::Tools::XmlUnorderedMap HLL::Tools::CreateMapFromXML(maxon::IoXmlNodeInterface * xmlHead)
{
	HLL::Tools::XmlUnorderedMap rval;

	auto it = xmlHead;
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
		if (it->GetDown())
		{
			rval.Emplace(CreateMapFromXML(it->GetDown()));
		}

		if (it->GetNext())
		{
			rval.Emplace(CreateMapFromXML(it->GetNext()));
		}
	}

	return rval;
}

Bool HLL::Tools::CheckForUpdates(String * link, const String & url)
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

	auto xml_map = CreateMapFromXML(res.GetValue());
	Int32 maj = xml_map["update.version.major"_s]->GetValue().ToInt32().GetValue();
	Int32 min = xml_map["update.version.minor"_s]->GetValue().ToInt32().GetValue();
	*link = xml_map["update.link"_s]->GetValue();

	if (maj >= HLL_VERSION_MAJOR)
	{
		if (min > HLL_VERSION_MINOR)
			return true;
	}

	return false;
}