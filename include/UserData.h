#pragma once

class UserProfile {
public:
	// all relatively confusing verbose names

	std::string m_name;
	std::string m_gamerTag;
	std::string m_networkUserId;

	UserProfile(std::string name,
		std::string gamerTag, 
		std::string networkUserId)
	: m_name(std::move(name)), m_gamerTag(std::move(gamerTag)), m_networkUserId(std::move(networkUserId)) {

	}

};
